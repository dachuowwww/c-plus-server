#include "RedisClient.h"
#include <cerrno>
#include <cstdlib>

RedisReplyPtr RedisClient::MakeReply(void *raw) {
  return RedisReplyPtr(static_cast<redisReply *>(raw), &freeReplyObject);
}

RedisContextPtr RedisClient::Connect(const std::string &host, int port, const std::string &password, int db) {
  const struct timeval timeout = {1, 0};
  RedisContextPtr context(redisConnectWithTimeout(host.c_str(), port, timeout), &redisFree);  // 登录获取上下文
  if (!context || context->err) {
    return RedisContextPtr(nullptr, &redisFree);
  }

  if (!password.empty()) {
    RedisReplyPtr auth_reply(static_cast<redisReply *>(
                                 redisCommand(context.get(), "AUTH %b", password.data(), password.size())),  // 检查密码
                             &freeReplyObject);
    if (!auth_reply || auth_reply->type == REDIS_REPLY_ERROR) {
      return RedisContextPtr(nullptr, &redisFree);
    }
  }

  if (db > 0) {
    RedisReplyPtr select_reply(static_cast<redisReply *>(redisCommand(context.get(), "SELECT %d", db)),  // 选择数据库
                               &freeReplyObject);
    if (!select_reply || select_reply->type == REDIS_REPLY_ERROR) {
      return RedisContextPtr(nullptr, &redisFree);
    }
  }

  return context;
}

RedisClient::RedisClient() : host_("127.0.0.1"), port_(6379), password_(""), db_(0) {
  const char *redis_host = std::getenv("REDIS_HOST");
  const char *redis_port = std::getenv("REDIS_PORT");
  const char *redis_password = std::getenv("REDIS_PASSWORD");
  const char *redis_db = std::getenv("REDIS_DB");

  host_ = (redis_host != nullptr) ? redis_host : "127.0.0.1";
  port_ = ParseIntOrDefault(redis_port, 6379);
  password_ = (redis_password != nullptr) ? redis_password : "";
  db_ = ParseIntOrDefault(redis_db, 0);
}

RedisClient::RedisClient(std::string host, int port, std::string password, int db)
    : host_(std::move(host)), port_(port), password_(std::move(password)), db_(db) {}

int RedisClient::ParseIntOrDefault(const char *text, int default_value) {
  if (text == nullptr || text[0] == '\0') {
    return default_value;
  }

  char *end = nullptr;
  errno = 0;
  const int32_t parsed = std::strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0') {
    return default_value;
  }
  return static_cast<int>(parsed);
}

bool RedisClient::Get(const std::string &key, std::string *value) {
  if (value == nullptr) {
    return false;
  }

  RedisContextPtr context = Connect(host_, port_, password_, db_);
  if (!context) {
    return false;
  }

  RedisReplyPtr reply =
      MakeReply(redisCommand(context.get(), "GET %b", key.data(), key.size()));  // RESP 协议格式化命令，二进制安全
  if (!reply || reply->type != REDIS_REPLY_STRING) {
    return false;
  }
  value->assign(reply->str, static_cast<size_t>(reply->len));
  return true;
}

bool RedisClient::SetEx(const std::string &key, int ttl_seconds, const std::string &value) {
  if (ttl_seconds <= 0) {
    return false;
  }

  RedisContextPtr context = Connect(host_, port_, password_, db_);
  if (!context) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(
      redisCommand(context.get(), "SETEX %b %d %b", key.data(), key.size(), ttl_seconds, value.data(), value.size()));
  return reply && reply->type == REDIS_REPLY_STATUS && std::string(reply->str, static_cast<size_t>(reply->len)) == "OK";
}

bool RedisClient::Del(const std::string &key) {
  RedisContextPtr context = Connect(host_, port_, password_, db_);
  if (!context) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context.get(), "DEL %b", key.data(), key.size()));
  return reply && reply->type == REDIS_REPLY_INTEGER && reply->integer >= 0;
}

int RedisClient::TTL(const std::string &key) {
  RedisContextPtr context = Connect(host_, port_, password_, db_);
  if (!context) {
    return -2;
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context.get(), "TTL %b", key.data(), key.size()));
  if (!reply || reply->type != REDIS_REPLY_INTEGER) {
    return -2;
  }
  return static_cast<int>(reply->integer);
}

bool RedisClient::ZIncrBy(const std::string &key, double increment, const std::string &member) {
  RedisContextPtr context = Connect(host_, port_, password_, db_);
  if (!context) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(
      redisCommand(context.get(), "ZINCRBY %b %f %b", key.data(), key.size(), increment, member.data(), member.size()));
  return reply && (reply->type == REDIS_REPLY_STRING || reply->type == REDIS_REPLY_STATUS);
}

std::vector<std::string> RedisClient::ZRevRange(const std::string &key, int start, int stop) {
  RedisContextPtr context = Connect(host_, port_, password_, db_);
  if (!context) {
    return {};
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context.get(), "ZREVRANGE %b %d %d", key.data(), key.size(), start,
                                               stop));  // 从大到小取 top_n 个元素，包含 stop
  if (!reply || reply->type != REDIS_REPLY_ARRAY) {
    return {};
  }

  std::vector<std::string> result;
  result.reserve(static_cast<size_t>(reply->elements));
  for (size_t i = 0; i < reply->elements; ++i) {
    const redisReply *item = reply->element[i];
    if (item != nullptr && item->type == REDIS_REPLY_STRING) {
      result.emplace_back(item->str, static_cast<size_t>(item->len));
    }
  }
  return result;
}

bool RedisClient::TryLock(const std::string &key, const std::string &token, int ttl_seconds) {
  if (ttl_seconds <= 0) {
    return false;
  }

  RedisContextPtr context = Connect(host_, port_, password_, db_);
  if (!context) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context.get(), "SET %b %b NX EX %d", key.data(), key.size(),
                                               token.data(), token.size(), ttl_seconds));
  return reply && reply->type == REDIS_REPLY_STATUS && std::string(reply->str, static_cast<size_t>(reply->len)) == "OK";
}

bool RedisClient::Unlock(const std::string &key, const std::string &token) {
  static const char kUnlockScript[] =
      "if redis.call('GET', KEYS[1]) == ARGV[1] then "  // Lua脚本
      "return redis.call('DEL', KEYS[1]) else return 0 end";

  RedisContextPtr context = Connect(host_, port_, password_, db_);
  if (!context) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context.get(), "EVAL %s 1 %b %b", kUnlockScript, key.data(), key.size(),
                                               token.data(), token.size()));
  return reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
}
