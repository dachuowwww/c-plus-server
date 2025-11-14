#pragma once
#include <arpa/inet.h>
#include "Macro.h"
class InetAddress {
 public:
  InetAddress(const char *ip, uint16_t port);
  ~InetAddress() = default;

  [[nodiscard]] char *GetIP() const;
  [[nodiscard]] uint16_t GetPort() const;

  private:
  sockaddr_in addr_ = {};
  socklen_t addr_len_ = 0;

  DISALLOW_COPY_AND_ASSIGN(InetAddress);
};
