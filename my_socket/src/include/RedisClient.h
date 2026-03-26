#pragma once

#include <hiredis/hiredis.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "Macro.h"

using RedisContextPtr = std::unique_ptr<redisContext, decltype(&redisFree)>;  // 专门函数释放
using RedisReplyPtr = std::unique_ptr<redisReply, decltype(&freeReplyObject)>;
class RedisClient {
 public:
  enum class GetResult {
    kHit,
    kMiss,
    kError,
  };

  RedisClient();
  RedisClient(std::string host, int port, std::string password = "", int db = 0);
  ~RedisClient();

  GetResult GetWithStatus(const std::string &key, std::string *value);
  bool Get(const std::string &key, std::string *value);
  bool SetEx(const std::string &key, int ttl_seconds, const std::string &value);
  bool Expire(const std::string &key, int ttl_seconds);
  bool Del(const std::string &key);
  int TTL(const std::string &key);
  bool ZIncrBy(const std::string &key, double increment, const std::string &member);
  std::vector<std::string> ZRevRange(const std::string &key, int start, int stop);
  bool TryLock(const std::string &key, const std::string &token, int ttl_seconds);
  bool Unlock(const std::string &key, const std::string &token);

 private:
  static int ParseIntOrDefault(const char *text, int default_value);
  RedisReplyPtr MakeReply(void *raw);
  RedisContextPtr Connect(const std::string &host, int port, const std::string &password, int db);
  redisContext *AcquireContext();
  void ReleaseContext(redisContext *context, bool reusable);
  bool IsContextReusable(redisContext *context, const redisReply *reply) const;
  void ClearPool();

  std::string host_;
  int port_;
  std::string password_;
  int db_;
  size_t pool_size_;
  int in_use_count_;
  std::mutex pool_mutex_;
  std::condition_variable pool_cv_;
  std::vector<redisContext *> pool_;

  DISALLOW_COPY_AND_ASSIGN(RedisClient);
};
