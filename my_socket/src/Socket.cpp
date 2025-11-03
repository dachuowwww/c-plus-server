#include "Socket.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include "Error.h"
#include "InetAddress.h"
#include "ThreadPool.h"

using std::cout;
using std::endl;
using std::shared_ptr;
TCPSocket::TCPSocket(shared_ptr<InetAddress> InetAddr) : addr_(InetAddr), addr_size_(sizeof(*(addr_->AddrEntity()))) {
  sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  Errif(sock_fd_ == -1, "socket create error");
}

TCPSocket::~TCPSocket() {}

int TCPSocket::GetFd() const { return sock_fd_; }

char *TCPSocket::GetIP() const { return addr_->GetIP(); }

uint16_t TCPSocket::GetPort() const { return addr_->GetPort(); }
void TCPSocket::Listen() const { Errif(::listen(sock_fd_, SOMAXCONN) == -1, "socket listen error"); }

void TCPSocket::Bind() {
  Errif(::bind(sock_fd_, (sockaddr *)addr_->AddrEntity(), addr_size_) == -1, "socket bind error");
}

void TCPSocket::SetNonblocking() const { fcntl(sock_fd_, F_SETFL, fcntl(sock_fd_, F_GETFL) | O_NONBLOCK); }

void TCPSocket::Accept(const int serv_fd) {
  sock_fd_ = ::accept(serv_fd, (sockaddr *)addr_->AddrEntity(), &addr_size_);
  Errif(sock_fd_ == -1, "socket accept error");
  cout << "new client fd " << sock_fd_ << "! IP: " << addr_->GetIP() << " Port:" << addr_->GetPort() << endl;
}

void TCPSocket::Connect() const {
  Errif(::connect(sock_fd_, (sockaddr *)addr_->AddrEntity(), addr_size_) == -1, "socket connect error");
}
