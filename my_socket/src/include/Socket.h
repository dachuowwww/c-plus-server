#pragma once
#include <arpa/inet.h>
#include "Macro.h"
class Socket {
 public:
  Socket();
  explicit Socket(int sock_fd);
  // Socket(int sock_fd, std::shared_ptr<InetAddress> InetAddr);
  ~Socket();
  [[nodiscard]] int GetFd() const;
  [[nodiscard]] bool IsNonBlocking() const;
  void Bind(const char *ip, uint16_t port);
  void Listen();
  void SetNonBlocking();
  int Accept();
  void Connect(const char *ip, uint16_t port);

 private:
  int sock_fd_ = -1;  // 自身的文件描述符

  DISALLOW_COPY_AND_ASSIGN(Socket);
};
