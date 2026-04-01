#include "RedisClient.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>

namespace {
constexpr int kPoolAcquireWaitMs = 200;

timeval MillisecondsToTimeval(int timeout_ms) {
  if (timeout_ms <= 0) {
    timeout_ms = 1;
  }
  timeval timeout{};
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;
  return timeout;
}
}  // namespace

RedisReplyPtr RedisClient::MakeReply(void *raw) {
  return RedisReplyPtr(static_cast<redisReply *>(raw), &freeReplyObject);
}

RedisContextPtr RedisClient::Connect(const std::string &host, int port, const std::string &password, int db) {
  const struct timeval timeout = MillisecondsToTimeval(connect_timeout_ms_);
  RedisContextPtr context(redisConnectWithTimeout(host.c_str(), port, timeout), &redisFree);
  if (!context || context->err) {
    return RedisContextPtr(nullptr, &redisFree);
  }
  const struct timeval command_timeout = MillisecondsToTimeval(command_timeout_ms_);
  if (redisSetTimeout(context.get(), command_timeout) != REDIS_OK) {
    return RedisContextPtr(nullptr, &redisFree);
  }

  if (!password.empty()) {
    RedisReplyPtr auth_reply(
        static_cast<redisReply *>(redisCommand(context.get(), "AUTH %b", password.data(), password.size())),
        &freeReplyObject);
    if (!auth_reply || auth_reply->type == REDIS_REPLY_ERROR) {
      return RedisContextPtr(nullptr, &redisFree);
    }
  }

  if (db > 0) {
    RedisReplyPtr select_reply(static_cast<redisReply *>(redisCommand(context.get(), "SELECT %d", db)),
                               &freeReplyObject);
    if (!select_reply || select_reply->type == REDIS_REPLY_ERROR) {
      return RedisContextPtr(nullptr, &redisFree);
    }
  }

  return context;
}
RedisClient *RedisClient::instance_ = nullptr;

RedisClient::RedisClient(std::string host, int port, std::string password, int db, int connect_timeout_ms,
                         int command_timeout_ms)
    : host_(std::move(host)),
      port_(port),
      password_(std::move(password)),
      db_(db),
      connect_timeout_ms_(connect_timeout_ms),
      command_timeout_ms_(command_timeout_ms) {}

void RedisClient::Init(std::string host, int port, std::string password, int db, int connect_timeout_ms,
                       int command_timeout_ms) {
  if (instance_ == nullptr) {
    instance_ = new RedisClient(host, port, password, db, connect_timeout_ms, command_timeout_ms);
  }
}

RedisClient &RedisClient::Instance() {
  if (instance_ == nullptr) {
    throw std::runtime_error("RedisClient 未初始化");
  }
  return *instance_;
}

// RedisClient::RedisClient() {
//   const char *redis_host = std::getenv("REDIS_HOST");
//   const char *redis_port = std::getenv("REDIS_PORT");
//   const char *redis_password = std::getenv("REDIS_PASSWORD");
//   const char *redis_db = std::getenv("REDIS_DB");
//   const char *redis_pool_size = std::getenv("REDIS_POOL_SIZE");

//   host_ = (redis_host != nullptr) ? redis_host : "127.0.0.1";
//   port_ = ParseIntOrDefault(redis_port, 6379);
//   password_ = (redis_password != nullptr) ? redis_password : "";
//   db_ = ParseIntOrDefault(redis_db, 0);
//   const int configured_pool_size = ParseIntOrDefault(redis_pool_size, kDefaultPoolSize);
//   if (configured_pool_size <= 0) {
//     pool_size_ = kDefaultPoolSize;
//   } else {
//     pool_size_ = static_cast<size_t>(configured_pool_size);
//   }
// }

// RedisClient::RedisClient(std::string host, int port, std::string password, int db)
//     : host_(std::move(host)),
//       port_(port),
//       password_(std::move(password)),
//       db_(db),
//       pool_size_(kDefaultPoolSize),
//       in_use_count_(0) {
//   const char *redis_pool_size = std::getenv("REDIS_POOL_SIZE");
//   const int configured_pool_size = ParseIntOrDefault(redis_pool_size, kDefaultPoolSize);
//   if (configured_pool_size <= 0) {
//     pool_size_ = kDefaultPoolSize;
//   } else {
//     pool_size_ = static_cast<size_t>(configured_pool_size);
//   }
// }

RedisClient::~RedisClient() { ClearPool(); }

// int RedisClient::ParseIntOrDefault(const char *text, int default_value) {
//   if (text == nullptr || text[0] == '\0') {
//     return default_value;
//   }

//   char *end = nullptr;
//   errno = 0;
//   const long parsed = std::strtol(text, &end, 10);
//   if (errno != 0 || end == text || *end != '\0') {
//     return default_value;
//   }
//   if (parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) {
//     return default_value;
//   }
//   return static_cast<int>(parsed);
// }

redisContext *RedisClient::AcquireContext() {
  std::unique_lock<std::mutex> lock(pool_mutex_);  // 锁内只做状态判断和名额预留
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kPoolAcquireWaitMs);

  while (true) {
    if (!pool_.empty()) {
      redisContext *context = pool_.back();
      pool_.pop_back();
      ++in_use_count_;
      return context;
    }

    if (static_cast<size_t>(in_use_count_) < pool_size_) {  // 预留名额，创建新链接，后续判断
      ++in_use_count_;                                      // 防并发
      lock.unlock();
      RedisContextPtr created = Connect(host_, port_, password_, db_);
      if (!created) {
        lock.lock();
        --in_use_count_;
        pool_cv_.notify_one();
        return nullptr;
      }
      return created.release();
    }

    if (pool_cv_.wait_until(lock, deadline) == std::cv_status::timeout) {  // 等待超时
      return nullptr;
    }
  }
}

void RedisClient::ReleaseContext(redisContext *context, bool reusable) {
  std::lock_guard<std::mutex> lock(pool_mutex_);
  if (in_use_count_ > 0) {
    --in_use_count_;
  }

  if (context != nullptr) {
    if (reusable) {
      pool_.push_back(context);
    } else {
      redisFree(context);
    }
  }
  pool_cv_.notify_one();
}

bool RedisClient::IsContextReusable(redisContext *context, const redisReply *reply) const {
  if (context == nullptr || context->err != 0) {
    return false;
  }
  return reply != nullptr;
}

void RedisClient::ClearPool() {
  std::lock_guard<std::mutex> lock(pool_mutex_);
  for (auto *context : pool_) {
    if (context != nullptr) {
      redisFree(context);
    }
  }
  pool_.clear();
}

RedisClient::GetResult RedisClient::GetWithStatus(const std::string &key, std::string *value) {
  if (value == nullptr) {
    return GetResult::kError;
  }

  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return GetResult::kError;
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context, "GET %b", key.data(), key.size()));
  const bool reusable = IsContextReusable(context, reply.get());
  GetResult result = GetResult::kError;
  if (reply && reply->type == REDIS_REPLY_STRING) {
    value->assign(reply->str, static_cast<size_t>(reply->len));
    result = GetResult::kHit;
  } else if (reply && reply->type == REDIS_REPLY_NIL) {
    result = GetResult::kMiss;
  }
  ReleaseContext(context, reusable);
  return result;
}

bool RedisClient::Get(const std::string &key, std::string *value) {
  return GetWithStatus(key, value) == GetResult::kHit;
}
bool RedisClient::SetEx(const std::string &key, int ttl_seconds, const std::string &value) {
  if (ttl_seconds <= 0) {
    return false;
  }

  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(
      redisCommand(context, "SETEX %b %d %b", key.data(), key.size(), ttl_seconds, value.data(), value.size()));
  const bool reusable = IsContextReusable(context, reply.get());
  const bool ok =
      reply && reply->type == REDIS_REPLY_STATUS && std::string(reply->str, static_cast<size_t>(reply->len)) == "OK";
  ReleaseContext(context, reusable);
  return ok;
}

bool RedisClient::Expire(const std::string &key, int ttl_seconds) {
  if (ttl_seconds <= 0) {
    return false;
  }

  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context, "EXPIRE %b %d", key.data(), key.size(), ttl_seconds));
  const bool reusable = IsContextReusable(context, reply.get());
  const bool ok = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
  ReleaseContext(context, reusable);
  return ok;
}

bool RedisClient::Del(const std::string &key) {
  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context, "DEL %b", key.data(), key.size()));
  const bool reusable = IsContextReusable(context, reply.get());
  const bool ok = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer >= 0;
  ReleaseContext(context, reusable);
  return ok;
}

int RedisClient::TTL(const std::string &key) {
  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return -2;
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context, "TTL %b", key.data(), key.size()));
  const bool reusable = IsContextReusable(context, reply.get());
  int ttl = -2;
  if (reply && reply->type == REDIS_REPLY_INTEGER) {
    ttl = static_cast<int>(reply->integer);
  }
  ReleaseContext(context, reusable);
  return ttl;
}

bool RedisClient::IncrBy(const std::string &key, int64_t delta) {
  int64_t value = 0;
  return IncrByWithValue(key, delta, &value);
}

bool RedisClient::IncrByWithValue(const std::string &key, int64_t delta, int64_t *new_value) {
  if (new_value == nullptr) {
    return false;
  }
  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return false;
  }

  RedisReplyPtr reply =
      MakeReply(redisCommand(context, "INCRBY %b %lld", key.data(), key.size(), static_cast<uint64_t>(delta)));
  const bool reusable = IsContextReusable(context, reply.get());
  const bool ok = reply && reply->type == REDIS_REPLY_INTEGER;
  if (ok) {
    *new_value = reply->integer;
  }
  ReleaseContext(context, reusable);
  return ok;
}

bool RedisClient::ZIncrBy(const std::string &key, double increment, const std::string &member) {
  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(
      redisCommand(context, "ZINCRBY %b %f %b", key.data(), key.size(), increment, member.data(), member.size()));
  const bool reusable = IsContextReusable(context, reply.get());
  const bool ok = reply && (reply->type == REDIS_REPLY_STRING || reply->type == REDIS_REPLY_STATUS);
  ReleaseContext(context, reusable);
  return ok;
}

std::vector<std::string> RedisClient::ZRevRange(const std::string &key, int start, int stop) {
  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return {};
  }

  RedisReplyPtr reply = MakeReply(redisCommand(context, "ZREVRANGE %b %d %d", key.data(), key.size(), start, stop));
  const bool reusable = IsContextReusable(context, reply.get());
  std::vector<std::string> result;
  if (reply && reply->type == REDIS_REPLY_ARRAY) {
    result.reserve(static_cast<size_t>(reply->elements));
    for (size_t i = 0; i < reply->elements; ++i) {
      const redisReply *item = reply->element[i];
      if (item != nullptr && item->type == REDIS_REPLY_STRING) {
        result.emplace_back(item->str, static_cast<size_t>(item->len));
      }
    }
  }
  ReleaseContext(context, reusable);
  return result;
}

bool RedisClient::TryLock(const std::string &key, const std::string &token, int ttl_seconds) {
  if (ttl_seconds <= 0) {
    return false;
  }

  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(
      redisCommand(context, "SET %b %b NX EX %d", key.data(), key.size(), token.data(), token.size(), ttl_seconds));
  const bool reusable = IsContextReusable(context, reply.get());
  const bool ok =
      reply && reply->type == REDIS_REPLY_STATUS && std::string(reply->str, static_cast<size_t>(reply->len)) == "OK";
  ReleaseContext(context, reusable);
  return ok;
}

bool RedisClient::Unlock(const std::string &key, const std::string &token) {
  static const char kUnlockScript[] =
      "if redis.call('GET', KEYS[1]) == ARGV[1] then "
      "return redis.call('DEL', KEYS[1]) else return 0 end";

  redisContext *context = AcquireContext();
  if (context == nullptr) {
    return false;
  }

  RedisReplyPtr reply = MakeReply(
      redisCommand(context, "EVAL %s 1 %b %b", kUnlockScript, key.data(), key.size(),
                   token.data(),  // lua脚本里key和arg都用二进制安全的方式传递，避免token里有特殊字符导致的问题
                   token.size()));
  const bool reusable = IsContextReusable(context, reply.get());
  const bool ok = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
  ReleaseContext(context, reusable);
  return ok;
}
