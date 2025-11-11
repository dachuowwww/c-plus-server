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
Socket::Socket(shared_ptr<InetAddress> InetAddr) : addr_(std::move(InetAddr)) {
  addr_size_ = sizeof(*(addr_->AddrEntity()));
  sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  Errif(sock_fd_ == -1, "socket create error");
}

// Socket::Socket(int sock_fd, shared_ptr<InetAddress> InetAddr) : sock_fd_(sock_fd), addr_(std::move(InetAddr)) {
//   addr_size_ = sizeof(*(addr_->AddrEntity()));  // 未Bind
//   Errif(sock_fd_ == -1, "socket create error");
// }

Socket::~Socket() {
  if (sock_fd_ != -1) {
    ::close(sock_fd_);
    sock_fd_ = -1;
  }
}
int Socket::GetFd() const { return sock_fd_; }

char *Socket::GetIP() const { return addr_->GetIP(); }

uint16_t Socket::GetPort() const { return addr_->GetPort(); }
void Socket::Listen() const { Errif(::listen(sock_fd_, SOMAXCONN) == -1, "socket listen error"); }

void Socket::Bind() { Errif(::bind(sock_fd_, (sockaddr *)addr_->AddrEntity(), addr_size_) == -1, "socket bind error"); }

void Socket::SetNonBlocking() const { fcntl(sock_fd_, F_SETFL, fcntl(sock_fd_, F_GETFL) | O_NONBLOCK); }

void Socket::Accept(const shared_ptr<Socket> &serv_socket) {
  ssize_t clnt_fd = -1;
  if (serv_socket->IsNonBlocking()) {  // 防止瞬时队列为空造成阻塞 非阻塞模式主要适合监听线程要同时处理多件事，或使用 ET
    std::cout << "nonblocking socket accept" << std::endl;
    while (true) {
      clnt_fd = ::accept(serv_socket->GetFd(), (sockaddr *)addr_->AddrEntity(), &addr_size_);
      if (clnt_fd >= 0) {
        break;
      }
      if (clnt_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        continue;
      }
      Errif(true, "nonblocking socket accept error");
    }
  } else {
    std::cout << "blocking socket accept" << std::endl;
    clnt_fd = ::accept(serv_socket->GetFd(), (sockaddr *)addr_->AddrEntity(), &addr_size_);
    Errif(clnt_fd == -1, "blocking socket accept error");
  }
  sock_fd_ = clnt_fd;  // 保存接受到的客户端的文件描述符
  cout << "new client fd " << clnt_fd << "! IP: " << addr_->GetIP() << " Port:" << addr_->GetPort() << endl;
  // 处理客户端连接请求
}
void Socket::Connect() {
  if (Socket::IsNonBlocking()) {  // 自身非阻塞连接也会马上返回，不会阻塞
    std::cout << "nonblocking socket connect" << std::endl;
    ssize_t conn_err = connect(sock_fd_, (sockaddr *)addr_->AddrEntity(), addr_size_);
    if (conn_err == -1 && errno == EINPROGRESS) {
      while (true) {
        fd_set write_set;  // 是 select() 系统调用使用的文件描述符集合类型
        // NOLINTNEXTLINE(hicpp-no-assembler, readability-isolate-declaration)
        FD_ZERO(&write_set);
        // 清空集合
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        FD_SET(sock_fd_, &write_set);
        // 将 sock_fd_ 加入集合
        int select_err =
            select(sock_fd_ + 1, nullptr, &write_set, nullptr, nullptr);  // 调用 select() 系统调用，等待 sock_fd_ 可写
        if (select_err > 0) {
          break;
        }
        if (select_err == -1 && errno == EINTR) {
          continue;
        }
        close(sock_fd_);
        Errif(true, "socket connect timeout");
      }
      int error = 0;
      socklen_t len = sizeof(error);
      getsockopt(sock_fd_, SOL_SOCKET, SO_ERROR, &error,
                 &len);  // SOL_SOCKET表示要在“socket 层”上操作选项，SO_ERROR表示获取最近一次 socket 操作的错误状态。
      if (error == 0) {
        return;
      }
      close(sock_fd_);
      Errif(true, "socket getsockopt error");
    }
  } else {
    std::cout << "blocking socket connect" << std::endl;
    Errif(connect(sock_fd_, (sockaddr *)addr_->AddrEntity(), addr_size_) == -1, "socket connect error");
  }
}

bool Socket::IsNonBlocking() const { return (fcntl(sock_fd_, F_GETFL) & O_NONBLOCK) != 0; }
