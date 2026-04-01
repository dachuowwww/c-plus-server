#include "SessionManager.h"

#include <random>
#include <sstream>

#include "RedisClient.h"

namespace {
char ToHexDigit(unsigned int value) { return static_cast<char>((value < 10U) ? ('0' + value) : ('a' + (value - 10U))); }
}  // namespace

SessionManager::SessionManager(int ttl_seconds) : ttl_seconds_(ttl_seconds > 0 ? ttl_seconds : 30 * 60) {}

std::string SessionManager::CreateUserSession(int64_t user_id, const std::string &username) {
  if (user_id <= 0 || username.empty()) {
    return "";
  }

  const std::string token = GenerateToken();
  const std::string key = "session:" + token;
  UserSession session;
  session.user_id = user_id;
  session.username = username;

  if (!RedisClient::Instance().SetEx(key, ttl_seconds_, EncodeSession(session))) {
    return "";
  }
  return token;
}

bool SessionManager::GetUserSession(const std::string &token, UserSession *session) {
  if (session == nullptr || token.empty()) {
    return false;
  }

  std::string raw;
  if (!RedisClient::Instance().Get("session:" + token, &raw)) {
    return false;
  }
  return DecodeSession(raw, session);
}

bool SessionManager::DeleteSession(const std::string &token) {
  if (token.empty()) {
    return false;
  }
  return RedisClient::Instance().Del("session:" + token);
}

bool SessionManager::RefreshSession(const std::string &token, int ttl_seconds) {
  if (token.empty() || ttl_seconds <= 0) {
    return false;
  }
  return RedisClient::Instance().Expire("session:" + token, ttl_seconds);
}

std::string SessionManager::GenerateToken() {
  static thread_local std::mt19937_64 rng(std::random_device {}());
  std::uniform_int_distribution<unsigned int> byte_dist(0, 255);

  std::string token;
  token.resize(64);
  for (size_t i = 0; i < 32; ++i) {
    const unsigned int byte = byte_dist(rng);
    token[2 * i] = ToHexDigit((byte >> 4U) & 0x0fU);
    token[2 * i + 1] = ToHexDigit(byte & 0x0fU);
  }
  return token;
}

std::string SessionManager::EncodeSession(const UserSession &session) {
  return std::to_string(session.user_id) + "\n" + session.username;
}

bool SessionManager::DecodeSession(const std::string &raw, UserSession *session) {
  if (session == nullptr) {
    return false;
  }
  const size_t split = raw.find('\n');
  if (split == std::string::npos) {
    return false;
  }

  try {
    session->user_id = std::stoll(raw.substr(0, split));
  } catch (...) {
    return false;
  }
  session->username = raw.substr(split + 1);
  return session->user_id > 0 && !session->username.empty();
}
