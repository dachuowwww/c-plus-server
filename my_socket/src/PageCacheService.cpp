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
  const RedisClient::GetResult first_get = redis_.GetWithStatus(key, &html);
  if (first_get == RedisClient::GetResult::kHit) {
    PutLocalShadow(key, ttl_seconds, html);
    worker.Hit(key, 1.0);
    return html;
  }
  if (first_get == RedisClient::GetResult::kError) {
    if (TryGetLocalShadow(key, &html)) {
      return html;
    }
    // Redis 异常时不触发回源构建，避免高并发下放大文件读取与重建抖动。
    return "";
  }

  // 避免缓存击穿，只有一个线程去构建页面，其他线程等待
  const std::string lock_key = "lock:" + key;
  const std::string token = GenToken();
  if (redis_.TryLock(lock_key, token, 5)) {
    std::string page;
    try {
      page = builder();
    } catch (...) {
      redis_.Unlock(lock_key, token);
      throw;
    }
    if (redis_.SetEx(key, ttl_seconds, page)) {
      worker.RegisterPage(key, ttl_seconds);
    }
    PutLocalShadow(key, ttl_seconds, page);
    redis_.Unlock(lock_key, token);
    worker.Hit(key, 1.0);
    return page;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  const RedisClient::GetResult second_get = redis_.GetWithStatus(key, &html);
  if (second_get == RedisClient::GetResult::kHit) {
    PutLocalShadow(key, ttl_seconds, html);
    worker.Hit(key, 1.0);
    return html;
  }
  if (second_get == RedisClient::GetResult::kError) {
    if (TryGetLocalShadow(key, &html)) {
      return html;
    } 
    return "";
  }
  // 保险
  std::string page = builder();
  PutLocalShadow(key, ttl_seconds, page);
  return page;
}

void PageCacheService::InvalidateFileListPages() {
  redis_.Del("page:filelist:user");
  redis_.Del("page:filelist:guest");
  RemoveLocalShadow("page:filelist:user");
  RemoveLocalShadow("page:filelist:guest");
}

bool PageCacheService::TryGetLocalShadow(const std::string &key, std::string *html) {
  if (html == nullptr) {
    return false;
  }
  const auto now = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(local_shadow_mutex_);
  auto it = local_shadow_.find(key);
  if (it == local_shadow_.end()) {
    return false;
  }
  if (it->second.expire_at <= now) { // 本地缓存过期了，删除掉
    local_shadow_.erase(it);
    return false;
  }
  *html = it->second.html;
  return true;
}

void PageCacheService::PutLocalShadow(const std::string &key, int ttl_seconds, const std::string &html) {
  if (key.empty() || ttl_seconds <= 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(local_shadow_mutex_);
  local_shadow_[key] = LocalShadowEntry{
      html,
      std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds),
  };
}

void PageCacheService::RemoveLocalShadow(const std::string &key) {
  std::lock_guard<std::mutex> lock(local_shadow_mutex_);
  local_shadow_.erase(key);
}

std::string PageCacheService::GenToken() const {
  static thread_local std::mt19937_64 rng(std::random_device{}());
  std::uniform_int_distribution<int64_t> dist;
  std::ostringstream oss;
  oss << std::hex << std::chrono::steady_clock::now().time_since_epoch().count() << "-" << dist(rng)
      << "-"  // 时间戳计数-随机数-线程ID哈希
      << std::hash<std::thread::id>{}(std::this_thread::get_id());
  return oss.str();
}
