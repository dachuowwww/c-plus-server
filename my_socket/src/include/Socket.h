#pragma once
#include <arpa/inet.h>
#include <memory>
using std::shared_ptr;
class InetAddress;
class TCPSocket {
 private:
  int sock_fd_;
  shared_ptr<InetAddress> addr_;
  socklen_t addr_size_;

 public:
  explicit TCPSocket(shared_ptr<InetAddress>);
  ~TCPSocket();
  TCPSocket(const TCPSocket &) = delete;
  TCPSocket &operator=(const TCPSocket &) = delete;
  TCPSocket(TCPSocket &&) = delete;
  TCPSocket &operator=(TCPSocket &&) = delete;
  [[nodiscard]] int GetFd() const;
  [[nodiscard]] char *GetIP() const;
  [[nodiscard]] uint16_t GetPort() const;
  void Listen() const;
  void Bind();
  void SetNonblocking() const;
  void Accept(const int);
  void Connect() const;
};
