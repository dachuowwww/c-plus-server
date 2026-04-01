#include "WarmupWorker.h"
#include <chrono>
#include "RedisClient.h"
namespace {
constexpr char kHotPagesKey[] = "hot_pages";
constexpr int kTopN = 5;
constexpr int kRefreshThresholdSeconds = 15;
constexpr int kRefreshIntervalSeconds = 30;
constexpr int kHitFlushIntervalMs = 1000;
}  // namespace

void WarmupWorker::Hit(const std::string &target, double score) {
  if (target.empty() || score == 0.0) {
    return;
  }
  RedisClient::Instance().ZIncrBy(kHotPagesKey, score, target);
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

void WarmupWorker::RegisterPage(const std::string &key, int ttl_seconds) {
  if (key.empty() || ttl_seconds <= 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (specs_.find(key) != specs_.end()) {
    return;
  }
  specs_[key] = ttl_seconds;
}

void WarmupWorker::Start() {
  if (worker_.joinable()) {
    return;
  }
  stop_.store(false);
  worker_ = std::thread(&WarmupWorker::Run, this);
}

void WarmupWorker::RefreshHotPages() {
  const auto top_pages = RedisClient::Instance().ZRevRange(kHotPagesKey, 0, kTopN - 1);
  for (const auto &key : top_pages) {
    int ttl_seconds;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = specs_.find(key);
      if (it == specs_.end()) {
        continue;
      }
      ttl_seconds = it->second;
    }

    const int ttl = RedisClient::Instance().TTL(key);
    if (ttl > 0 && ttl < kRefreshThresholdSeconds) {
      // 只刷新过期时间，不重建/重写值，避免后台重复打开文件或构造页面。
      RedisClient::Instance().Expire(key, ttl_seconds);
    }
  }
}

void WarmupWorker::Run() {
  auto last_refresh = std::chrono::steady_clock::now();
  while (!stop_.load()) {
    const auto now = std::chrono::steady_clock::now();
    if (now - last_refresh >= std::chrono::seconds(kRefreshIntervalSeconds)) {
      RefreshHotPages();
      last_refresh = now;
    }

    for (int i = 0; i < kHitFlushIntervalMs / 100 && !stop_.load(); ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}
