#pragma once
#include <arpa/inet.h>
#include "Macro.h"
class InetAddress {
 private:
  sockaddr_in addr_ = {};
  socklen_t addr_len_ = 0;

 public:
  InetAddress(const char *ip, uint16_t port);
  InetAddress();
  ~InetAddress() = default;
  sockaddr_in *AddrEntity();

  [[nodiscard]] char *GetIP() const;
  [[nodiscard]] uint16_t GetPort() const;

  DISALLOW_COPY_AND_ASSIGN(InetAddress);
};
