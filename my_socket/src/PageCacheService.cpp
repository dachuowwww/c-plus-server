#include "PageCacheService.h"
#include <chrono>
#include <random>
#include <sstream>
#include <thread>
#include "RedisClient.h"
#include "WarmupWorker.h"

PageCacheService &PageCacheService::Instance() {
  static PageCacheService service;
  return service;
}

std::string PageCacheService::GetOrBuild(const std::string &key, int ttl_seconds,
                                         const std::function<std::string()> &builder) {
  WarmupWorker &worker = WarmupWorker::Instance();
  worker.RegisterPage(key, ttl_seconds);
  std::string html;
  const RedisClient::GetResult first_get = RedisClient::Instance().GetWithStatus(key, &html);
  if (first_get == RedisClient::GetResult::kHit) {
    worker.Hit(key, 1.0);
    return html;
  }
  if (first_get == RedisClient::GetResult::kError) {
    return builder();
  }

  // 避免缓存击穿，只有一个线程去构建页面，其他线程等待
  const std::string lock_key = "lock:" + key;
  const std::string token = GenToken();
  if (RedisClient::Instance().TryLock(lock_key, token, 5)) {
    std::string page;
    try {
      page = builder();
    } catch (...) {
      RedisClient::Instance().Unlock(lock_key, token);
      throw;
    }
    RedisClient::Instance().SetEx(key, ttl_seconds, page);
    RedisClient::Instance().Unlock(lock_key, token);
    worker.Hit(key, 1.0);
    return page;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  const RedisClient::GetResult second_get = RedisClient::Instance().GetWithStatus(key, &html);
  if (second_get == RedisClient::GetResult::kHit) {
    worker.Hit(key, 1.0);
    return html;
  }
  if (second_get == RedisClient::GetResult::kError) {
    return builder();
  }
  // 保险
  return builder();
}

void PageCacheService::InvalidateFileListPages() {
  RedisClient::Instance().Del("page:filelist:user");
  RedisClient::Instance().Del("page:filelist:guest");
}

std::string PageCacheService::GenToken() const {
  static thread_local std::mt19937_64 rng(std::random_device {}());
  std::uniform_int_distribution<int64_t> dist;
  std::ostringstream oss;
  oss << std::hex << std::chrono::steady_clock::now().time_since_epoch().count() << "-" << dist(rng)
      << "-"  // 时间戳计数-随机数-线程ID哈希
      << std::hash<std::thread::id>{}(std::this_thread::get_id());
  return oss.str();
}
