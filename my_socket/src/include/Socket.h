#pragma once
#include <arpa/inet.h>
#include <memory>
#include "Macro.h"
class InetAddress;
class Socket {
 private:
  int sock_fd_ = -1;                   // 自身的文件描述符
  std::shared_ptr<InetAddress> addr_;  // 要连接的地址
  socklen_t addr_size_ = 0;            // 地址长度

 public:
  explicit Socket(std::shared_ptr<InetAddress> InetAddr);
  // Socket(int sock_fd, std::shared_ptr<InetAddress> InetAddr);
  ~Socket();
  [[nodiscard]] int GetFd() const;
  [[nodiscard]] char *GetIP() const;
  [[nodiscard]] uint16_t GetPort() const;
  [[nodiscard]] bool IsNonBlocking() const;
  void Listen() const;
  void Bind();
  void SetNonBlocking() const;
  void Accept(const std::shared_ptr<Socket> &serv_socket);
  void Connect();

  DISALLOW_COPY_AND_ASSIGN(Socket);
};
