#pragma once

#include <hiredis/hiredis.h>
#include <memory>
#include <string>
#include <vector>
#include "Macro.h"

using RedisContextPtr = std::unique_ptr<redisContext, decltype(&redisFree)>;  // 专门函数释放
using RedisReplyPtr = std::unique_ptr<redisReply, decltype(&freeReplyObject)>;
class RedisClient {
 public:
  RedisClient();
  RedisClient(std::string host, int port, std::string password = "", int db = 0);

  RedisReplyPtr MakeReply(void *raw);
  RedisContextPtr Connect(const std::string &host, int port, const std::string &password, int db);

  bool Get(const std::string &key, std::string *value);
  bool SetEx(const std::string &key, int ttl_seconds, const std::string &value);
  bool Del(const std::string &key);
  int TTL(const std::string &key);
  bool ZIncrBy(const std::string &key, double increment, const std::string &member);
  std::vector<std::string> ZRevRange(const std::string &key, int start, int stop);
  bool TryLock(const std::string &key, const std::string &token, int ttl_seconds);
  bool Unlock(const std::string &key, const std::string &token);

 private:
  static int ParseIntOrDefault(const char *text, int default_value);

  std::string host_;
  int port_;
  std::string password_;
  int db_;

  DISALLOW_COPY_AND_ASSIGN(RedisClient);
};
