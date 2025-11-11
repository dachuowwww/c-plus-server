#include "Server.h"
#include <unistd.h>
#include <iostream>
#include "Acceptor.h"
#include "Channel.h"
#include "Connection.h"
#include "Error.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "ThreadPool.h"

using std::cout;
using std::endl;
using std::function;
using std::make_shared;
using std::shared_ptr;
using std::thread;
Server::Server(shared_ptr<EventLoop> loop) : main_reactor_(std::move(loop)) {
  acceptor_ = std::make_unique<Acceptor>(main_reactor_);
  function<void(shared_ptr<Socket> &)> cb = std::bind(&Server::NewConnection, this, std::placeholders::_1);
  acceptor_->SetNewConnectionCallback(cb);

  unsigned int size = thread::hardware_concurrency();
  if (size <= 0) {
    size = 1;
  } else {
    thread_pool_ = std::make_unique<ThreadPool>(size);
    Errif(thread_pool_ == nullptr, "new threadPool error");
  }
  for (unsigned int i = 0; i < size; ++i) {
    subreactors_.emplace_back(std::make_shared<EventLoop>());
    Errif(subreactors_[i] == nullptr, "subReactors emplace_back error");
    function<void()> cb = std::bind(&EventLoop::Loop, subreactors_[i]);
    thread_pool_->Add(std::move(cb));
  }

  acceptor_->EnableListening();
}
Server::~Server() = default;

void Server::OnConnect(function<void(Connection *)> &&cb) { new_connection_callback_ = std::move(cb); }

void Server::NewConnection(const shared_ptr<Socket> &conn_socket) {
  int fd = conn_socket->GetFd();
  int idx = static_cast<int>(fd % subreactors_.size());
  auto clnt_conn = std::make_unique<Connection>(subreactors_[idx], conn_socket);
  // function<void(Connection *)> cb1 = std::bind(&Server::Handle, this, std::placeholders::_1);
  // 没有必要，因为Server::Handle已经绑定了this指针
  clnt_conn->SetHandleReadFunc(new_connection_callback_);
  function<void(shared_ptr<Socket> &)> cb2 = std::bind(&Server::RemoveConnection, this, std::placeholders::_1);
  clnt_conn->SetRemoveConnection(cb2);
  clnt_conn->SetET();
  clnt_conn->EnableReading();
  connections_[fd] = std::move(clnt_conn);
}

void Server::RemoveConnection(const shared_ptr<Socket> &conn_socket) {
  // ::close(conn_socket->GetFd());
  int fd = conn_socket->GetFd();
  connections_.erase(fd);
  cout << "client fd " << fd << " removed from connection map" << std::endl;
}

// void Server::OpenThreadPool(){
//     thread_pool.reset(new ThreadPool());
//     Errif(thread_pool == nullptr, "new threadPool error");
// }
