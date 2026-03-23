#include "WarmupWorker.h"
#include <chrono>

namespace {
constexpr char kHotPagesKey[] = "hot_pages";
constexpr int kTopN = 5;
constexpr int kRefreshThresholdSeconds = 15;
constexpr int kSleepIntervalSeconds = 30;
}  // namespace

void WarmupWorker::Hit(const std::string &target, double score) {
  if (target.empty()) {
    return;
  }
  redis_.ZIncrBy(kHotPagesKey, score, target);
}

WarmupWorker &WarmupWorker::Instance() {
  static WarmupWorker worker;
  return worker;
}

void WarmupWorker::Stop() {
  stop_.store(true);
  if (worker_.joinable()) {
    worker_.join();
  }
}
WarmupWorker::~WarmupWorker() { Stop(); }

void WarmupWorker::RegisterPage(const std::string &key, int ttl_seconds, Builder builder) {
  if (key.empty() || ttl_seconds <= 0 || !builder) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  specs_[key] = PageSpec{ttl_seconds, std::move(builder)};
}

void WarmupWorker::Start() {
  if (worker_.joinable()) {
    return;
  }
  stop_.store(false);
  worker_ = std::thread(&WarmupWorker::Run, this);
}

void WarmupWorker::Run() {
  while (!stop_.load()) {
    const auto top_pages = redis_.ZRevRange(kHotPagesKey, 0, kTopN - 1);
    for (const auto &key : top_pages) {
      PageSpec spec;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = specs_.find(key);
        if (it == specs_.end()) {
          continue;
        }
        spec = it->second;
      }

      const int ttl = redis_.TTL(key);
      if (ttl > 0 && ttl < kRefreshThresholdSeconds) {
        redis_.SetEx(key, spec.ttl_seconds, spec.builder());
      }
    }

    for (int i = 0; i < kSleepIntervalSeconds && !stop_.load(); ++i) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}
