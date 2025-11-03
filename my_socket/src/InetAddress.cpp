#include "InetAddress.h"

InetAddress::InetAddress(const char *ip,uint16_t port): addr_(), addr_len_() {
  addr_.sin_family = AF_INET;
  inet_pton(addr_.sin_family, ip, &addr_.sin_addr);
  addr_.sin_port = htons(port);
  addr_len_ = sizeof(addr_);
}
InetAddress::InetAddress() {
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_.sin_port = htons(0);
  addr_len_ = sizeof(addr_);
}
InetAddress::~InetAddress() = default;
sockaddr_in *InetAddress::AddrEntity() { return &addr_; }

char *InetAddress::GetIP() const { return inet_ntoa(addr_.sin_addr); }
uint16_t InetAddress::GetPort() const { return ntohs(addr_.sin_port); }
