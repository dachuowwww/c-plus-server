#pragma once

#include <hiredis/hiredis.h>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "Macro.h"
constexpr int kDefaultPoolSize = 16;
using RedisContextPtr = std::unique_ptr<redisContext, decltype(&redisFree)>;  // 专门函数释放
using RedisReplyPtr = std::unique_ptr<redisReply, decltype(&freeReplyObject)>;
class RedisClient {
 public:
  enum class GetResult {
    kHit,
    kMiss,
    kError,
  };

  static void Init(std::string host, int port, std::string password = "", int db = 0, int connect_timeout_ms = 100,
                   int command_timeout_ms = 100);

  // 获取实例
  ~RedisClient();
  static RedisClient &Instance();
  GetResult GetWithStatus(const std::string &key, std::string *value);
  bool Get(const std::string &key, std::string *value);
  bool SetEx(const std::string &key, int ttl_seconds, const std::string &value);
  bool Expire(const std::string &key, int ttl_seconds);
  bool Del(const std::string &key);
  int TTL(const std::string &key);
  bool IncrByWithValue(const std::string &key, int64_t delta, int64_t *new_value);
  bool IncrBy(const std::string &key, int64_t delta);
  bool ZIncrBy(const std::string &key, double increment, const std::string &member);
  std::vector<std::string> ZRevRange(const std::string &key, int start, int stop);
  bool TryLock(const std::string &key, const std::string &token, int ttl_seconds);
  bool Unlock(const std::string &key, const std::string &token);

 private:
  RedisClient(std::string host, int port, std::string password = "", int db = 0, int connect_timeout_ms = 100,
              int command_timeout_ms = 100);

  static RedisClient *instance_;

  // static int ParseIntOrDefault(const char *text, int default_value);
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
  int connect_timeout_ms_;
  int command_timeout_ms_;
  size_t pool_size_ = kDefaultPoolSize;
  int in_use_count_ = 0;
  std::mutex pool_mutex_;
  std::condition_variable pool_cv_;
  std::vector<redisContext *> pool_;

  DISALLOW_COPY_AND_ASSIGN(RedisClient);
};
