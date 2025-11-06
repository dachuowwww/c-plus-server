#include "Acceptor.h"  // cmake 文件写了引用子目录，所以这里引用头文件时不用写相对路径
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "Channel.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
using std::cout;
using std::endl;
using std::make_shared;
using std::shared_ptr;

Acceptor::Acceptor(EventLoop *loop) : loop_(loop) {
  listen_addr_ = make_shared<InetAddress>("127.0.0.1", 8888);
  accept_socket_ = make_shared<TCPSocket>(listen_addr_);
  accept_socket_->Bind();
  accept_socket_->Listen();

  accept_channel_ = std::make_unique<Channel>(loop_, accept_socket_->GetFd());
  std::function<void()> cb = std::bind(&Acceptor::Accept, this);
  accept_channel_->SetReadCallback(cb);
}

Acceptor::~Acceptor() { close(accept_socket_->GetFd()); }

void Acceptor::SetNewConnectionCallback(const std::function<void(shared_ptr<TCPSocket> &)> &cb) {
  new_connection_callback_ = cb;
}

void Acceptor::Accept() {
  // create an InetAddress storage for the accepted socket's peer info
  std::shared_ptr<InetAddress> clnt_addr = std::make_shared<InetAddress>();
  auto clnt_socket = make_shared<TCPSocket>(clnt_addr);
  clnt_socket->Accept(accept_socket_->GetFd());
  clnt_socket->SetNonBlocking();
  new_connection_callback_(clnt_socket);
}

void Acceptor::EnableListening() {
  accept_channel_
      ->EnableServReading();  // 监听连接事件，不使用ET模式,如果使用ET模式(EnableReading)，高并发下可能漏掉连接请求
  cout << "Server started at ip: " << accept_socket_->GetIP() << ",port: " << accept_socket_->GetPort() << endl;
}
bool Acceptor::IsInEpoll() const { return accept_channel_->IfInEpoll(); }

int Acceptor::GetFd() const { return accept_socket_->GetFd(); }
