#include "UserRepository.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <memory>
#include <stdexcept>
#include <utility>

#include "FileMetaRepository.h"
#include "RedisClient.h"

namespace {
constexpr int kUserCacheTtlSeconds = 60;

std::string BuildUserCacheValue(const UserRecord &record) {
  return std::to_string(record.id) + "\n" + record.username + "\n" + record.password_hash;
}

bool ParseUserCacheValue(const std::string &cache_value, UserRecord *record) {
  if (record == nullptr) {
    return false;
  }
  const size_t p0 = cache_value.find('\n');
  if (p0 == std::string::npos) {
    return false;
  }
  const size_t p1 = cache_value.find('\n', p0 + 1);
  if (p1 == std::string::npos) {
    return false;
  }

  try {
    record->id = std::stoll(cache_value.substr(0, p0));
  } catch (...) {
    return false;
  }
  record->username = cache_value.substr(p0 + 1, p1 - p0 - 1);
  record->password_hash = cache_value.substr(p1 + 1);
  return !record->username.empty();
}
}  // namespace

UserRepository *UserRepository::instance_ = nullptr;

UserRepository::UserRepository(std::string host, int port, std::string user, std::string password, std::string database,
                               int connect_timeout_sec, int read_timeout_sec, int write_timeout_sec,
                               int query_timeout_sec)
    : host_(std::move(host)),
      port_(port),
      user_(std::move(user)),
      password_(std::move(password)),
      database_(std::move(database)),
      connect_timeout_sec_(connect_timeout_sec),
      read_timeout_sec_(read_timeout_sec),
      write_timeout_sec_(write_timeout_sec),
      query_timeout_sec_(query_timeout_sec) {}

void UserRepository::Init(std::string host, int port, std::string user, std::string password, std::string database,
                          int connect_timeout_sec, int read_timeout_sec, int write_timeout_sec, int query_timeout_sec) {
  if (instance_ == nullptr) {
    instance_ = new UserRepository(std::move(host), port, std::move(user), std::move(password), std::move(database),
                                   connect_timeout_sec, read_timeout_sec, write_timeout_sec, query_timeout_sec);
  }
}

UserRepository &UserRepository::Instance() {
  if (instance_ == nullptr) {
    throw std::runtime_error("UserRepository 未初始化");
  }
  return *instance_;
}

bool UserRepository::GetUserByUsername(const std::string &username, UserRecord *record) const {
  if (record == nullptr || username.empty()) {
    return false;
  }

  const std::string cache_key = "user:cred:" + username;
  std::string cache_value;
  const RedisClient::GetResult redis_status = RedisClient::Instance().GetWithStatus(cache_key, &cache_value);
  if (redis_status == RedisClient::GetResult::kHit && ParseUserCacheValue(cache_value, record)) {
    return true;
  }

  sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
  sql::ConnectOptionsMap connection_options;
  connection_options["hostName"] = host_;
  connection_options["port"] = port_;
  connection_options["userName"] = user_;
  connection_options["password"] = password_;
  connection_options["OPT_CONNECT_TIMEOUT"] = connect_timeout_sec_;
  connection_options["OPT_READ_TIMEOUT"] = read_timeout_sec_;
  connection_options["OPT_WRITE_TIMEOUT"] = write_timeout_sec_;

  std::unique_ptr<sql::Connection> conn(driver->connect(connection_options));
  conn->setSchema(database_);

  std::unique_ptr<sql::PreparedStatement> stmt(
      conn->prepareStatement("SELECT id, username, password_hash FROM users WHERE username = ? LIMIT 1"));
  FileMetaRepository::Instance().TrySetQueryTimeout(stmt.get(), query_timeout_sec_);
  stmt->setString(1, username);

  std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());
  if (!rs->next()) {
    return false;
  }

  UserRecord db_record;
  db_record.id = rs->getInt64("id");
  db_record.username = rs->getString("username");
  db_record.password_hash = rs->getString("password_hash");
  *record = db_record;

  if (redis_status != RedisClient::GetResult::kError) {
    RedisClient::Instance().SetEx(cache_key, kUserCacheTtlSeconds, BuildUserCacheValue(db_record));
  }
  return true;
}

bool UserRepository::VerifyPassword(const std::string &username, const std::string &password_plain,
                                    UserRecord *record) const {
  UserRecord user;
  if (!GetUserByUsername(username, &user)) {
    return false;
  }

  if (user.password_hash != password_plain) {
    return false;
  }

  if (record != nullptr) {
    *record = user;
  }
  return true;
}
