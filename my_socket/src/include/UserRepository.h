#pragma once
#include <cstdint>
#include <string>
#include "Macro.h"

struct UserRecord {
  int64_t id = -1;
  std::string username;
  std::string password_hash;
};

class UserRepository {
 public:
  UserRepository();
  static void Init(std::string host, int port, std::string user, std::string password, std::string database,
                   int connect_timeout_sec = 1, int read_timeout_sec = 1, int write_timeout_sec = 1,
                   int query_timeout_sec = 1);
  static UserRepository &Instance();
  bool GetUserByUsername(const std::string &username, UserRecord *record) const;
  bool VerifyPassword(const std::string &username, const std::string &password_plain, UserRecord *record) const;

 private:
  UserRepository(std::string host, int port, std::string user, std::string password, std::string database,
                 int connect_timeout_sec, int read_timeout_sec, int write_timeout_sec, int query_timeout_sec);
  static UserRepository *instance_;
  std::string host_;
  int port_;
  std::string user_;
  std::string password_;
  std::string database_;
  int connect_timeout_sec_;
  int read_timeout_sec_;
  int write_timeout_sec_;
  int query_timeout_sec_;

  DISALLOW_COPY_AND_ASSIGN(UserRepository);
};
