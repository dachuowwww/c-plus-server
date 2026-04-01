#pragma once

#include <cstdint>
#include <string>

#include "Macro.h"

struct UserSession {
  int64_t user_id = -1;
  std::string username;
};

class SessionManager {
 public:
  explicit SessionManager(int ttl_seconds = 30 * 60);

  std::string CreateUserSession(int64_t user_id, const std::string &username);
  bool GetUserSession(const std::string &token, UserSession *session);
  bool DeleteSession(const std::string &token);
  bool RefreshSession(const std::string &token, int ttl_seconds);

 private:
  static std::string GenerateToken();
  static std::string EncodeSession(const UserSession &session);
  static bool DecodeSession(const std::string &raw, UserSession *session);

  int ttl_seconds_;

  DISALLOW_COPY_AND_ASSIGN(SessionManager);
};
