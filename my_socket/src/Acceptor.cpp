#include "Acceptor.h"  // cmake 文件写了引用子目录，所以这里引用头文件时不用写相对路径
#include <iostream>
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
using std::cout;
using std::endl;
using std::make_unique;

Acceptor::Acceptor(EventLoop *loop) : loop_(loop) {
  accept_socket_ = make_unique<Socket>();
  accept_socket_->Bind("127.0.0.1", 8888);  // 客户端为自动分配ip和端口的关系，服务端需要绑定固定的ip和端口
  accept_socket_->Listen();

  accept_channel_ = std::make_unique<Channel>(loop_, accept_socket_->GetFd());
  accept_channel_->SetReadCallback([this]() { this->Accept(); });
}

Acceptor::~Acceptor() = default;

void Acceptor::SetNewConnectionCallback(std::function<void(int)> &&cb) { new_connection_callback_ = std::move(cb); }

void Acceptor::Accept() { new_connection_callback_(accept_socket_->Accept()); }

void Acceptor::EnableListening() {
  accept_channel_
      ->EnableReading();  // 监听连接事件，不使用ET模式,如果使用ET模式(EnableReading)，高并发下可能漏掉连接请求
  // cout << "Server started at socket id: " << accept_socket_->GetFd() << endl;
}

bool Acceptor::IsInEpoll() const { return accept_channel_->IfInEpoll(); }

int Acceptor::GetFd() const { return accept_socket_->GetFd(); }
