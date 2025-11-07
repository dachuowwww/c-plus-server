#pragma once
#include <arpa/inet.h>
#include <memory>
#include "Macro.h"
class InetAddress;
class Socket {
 private:
  int sock_fd_ = -1;
  std::shared_ptr<InetAddress> addr_;
  socklen_t addr_size_ = 0;

 public:
  explicit Socket(std::shared_ptr<InetAddress> InetAddr);
  ~Socket() = default;
  [[nodiscard]] int GetFd() const;
  [[nodiscard]] char *GetIP() const;
  [[nodiscard]] uint16_t GetPort() const;
  [[nodiscard]] bool IsBlocking() const;
  void Listen() const;
  void Bind();
  void SetNonBlocking() const;
  void Accept(int serv_fd);
  void Connect() const;

  DISALLOW_COPY_AND_ASSIGN(Socket);
};
