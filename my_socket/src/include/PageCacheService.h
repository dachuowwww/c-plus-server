#pragma once

#include <functional>
#include <string>

#include "RedisClient.h"
#include "Macro.h"

class PageCacheService {
 public:
  static PageCacheService &Instance();

  std::string GetOrBuild(const std::string &key, int ttl_seconds, const std::function<std::string()> &builder);

  void InvalidateFileListPages();

 private:
  PageCacheService() = default;
  std::string GenToken() const;

  RedisClient redis_;

  DISALLOW_COPY_AND_ASSIGN(PageCacheService);
};
