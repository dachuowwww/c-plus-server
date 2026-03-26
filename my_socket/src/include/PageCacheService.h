#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "Macro.h"
#include "RedisClient.h"

class PageCacheService {
 public:
  static PageCacheService &Instance();

  std::string GetOrBuild(const std::string &key, int ttl_seconds, const std::function<std::string()> &builder);

  void InvalidateFileListPages();

 private:
  struct LocalShadowEntry {
    std::string html;
    std::chrono::steady_clock::time_point expire_at;
  };

  PageCacheService() = default;
  std::string GenToken() const;
  bool TryGetLocalShadow(const std::string &key, std::string *html);
  void PutLocalShadow(const std::string &key, int ttl_seconds, const std::string &html);
  void RemoveLocalShadow(const std::string &key);

  RedisClient redis_;
  std::mutex local_shadow_mutex_;
  std::unordered_map<std::string, LocalShadowEntry> local_shadow_;

  DISALLOW_COPY_AND_ASSIGN(PageCacheService);
};
