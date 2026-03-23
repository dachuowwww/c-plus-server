#pragma once
#include <string>
#include "Macro.h"

class UserRepository {
 public:
  UserRepository(std::string host, int port, std::string user, std::string password, std::string database);

  bool VerifyPlainPassword(const std::string &username, const std::string &password) const;

 private:
  std::string url_;
  std::string user_;
  std::string password_;
  std::string database_;

  DISALLOW_COPY_AND_ASSIGN(UserRepository);
};
