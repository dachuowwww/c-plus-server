#pragma once
#include <arpa/inet.h>
class InetAddress {
 private:
  sockaddr_in addr_;
  socklen_t addr_len_;

 public:
  InetAddress(const char *, const uint16_t);
  InetAddress();
  ~InetAddress();
  InetAddress(const InetAddress &) = delete;
  InetAddress &operator=(const InetAddress &) = delete;
  InetAddress(InetAddress &&) = delete;
  InetAddress &operator=(InetAddress &&) = delete;
  sockaddr_in *AddrEntity();

  [[nodiscard]] char *GetIP() const;
  [[nodiscard]] uint16_t GetPort() const;
};
