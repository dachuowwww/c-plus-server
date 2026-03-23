#include "PageCacheService.h"
#include <chrono>
#include <random>
#include <sstream>
#include <thread>
#include "WarmupWorker.h"

PageCacheService &PageCacheService::Instance() {
  static PageCacheService service;
  return service;
}

std::string PageCacheService::GetOrBuild(const std::string &key, int ttl_seconds,
                                         const std::function<std::string()> &builder) {
  WarmupWorker &worker = WarmupWorker::Instance();
  std::string html;
  if (redis_.Get(key, &html)) {
    worker.Hit(key, 1.0);
    return html;
  }
  // 避免缓存击穿，只有一个线程去构建页面，其他线程等待
  const std::string lock_key = "lock:" + key;
  const std::string token = GenToken();
  if (redis_.TryLock(lock_key, token, 5)) {  // 加五秒锁
    std::string page;
    try {
      page = builder();
    } catch (...) {
      redis_.Unlock(lock_key, token);
      throw;
    }
    worker.RegisterPage(key, ttl_seconds, builder);  // 注册到预热系统，后续自动刷新
    redis_.SetEx(key, ttl_seconds, page);
    redis_.Unlock(lock_key, token);
    worker.Hit(key, 1.0);
    return page;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if (redis_.Get(key, &html)) {
    worker.Hit(key, 1.0);
    return html;
  }
  // 保险
  std::string page = builder();
  worker.Hit(key, 1.0);
  return page;
}

void PageCacheService::InvalidateFileListPages() {
  redis_.Del("page:filelist:user");
  redis_.Del("page:filelist:guest");
}

std::string PageCacheService::GenToken() const {
  static thread_local std::mt19937_64 rng(std::random_device{ }());
  std::uniform_int_distribution<int64_t> dist;
  std::ostringstream oss;
  oss << std::hex << std::chrono::steady_clock::now().time_since_epoch().count() << "-" << dist(rng)
      << "-"  // 时间戳计数-随机数-线程ID哈希
      << std::hash<std::thread::id>{}(std::this_thread::get_id());
  return oss.str();
}
