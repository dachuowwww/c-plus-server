#pragma once
#include <arpa/inet.h>
#include <memory>
class InetAddress;
class TCPSocket {
 private:
  int sock_fd_ = -1;
  std::shared_ptr<InetAddress> addr_;
  socklen_t addr_size_ = 0;

 public:
  explicit TCPSocket(std::shared_ptr<InetAddress> InetAddr);
  ~TCPSocket() = default;
  TCPSocket(const TCPSocket &) = delete;
  TCPSocket &operator=(const TCPSocket &) = delete;
  TCPSocket(TCPSocket &&) = delete;
  TCPSocket &operator=(TCPSocket &&) = delete;
  [[nodiscard]] int GetFd() const;
  [[nodiscard]] char *GetIP() const;
  [[nodiscard]] uint16_t GetPort() const;
  [[nodiscard]] bool IsBlocking() const;
  void Listen() const;
  void Bind();
  void SetNonBlocking() const;
  void Accept(int serv_fd);
  void Connect() const;
};
