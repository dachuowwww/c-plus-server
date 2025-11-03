#include "Server.h"
#include <unistd.h>
#include <functional>
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
Server::Server(EventLoop *loop) : main_reactor_(loop) {
  acceptor_ = std::make_unique<Acceptor>(loop);
  function<void(shared_ptr<TCPSocket>)> cb = std::bind(&Server::NewConnection, this, std::placeholders::_1);
  acceptor_->SetNewConnectionCallback(cb);

  int size = thread::hardware_concurrency();
  if (size <= 0) {
    size = 1;
  } else {
    thread_pool_ = std::make_unique<ThreadPool>(size);
    Errif(thread_pool_ == nullptr, "new threadPool error");
  }
  for (int i = 0; i < size; ++i) {
    subreactors_.emplace_back(std::make_shared<EventLoop>());
    Errif(subreactors_[i] == nullptr, "subReactors emplace_back error");
    function<void()> cd = std::bind(&EventLoop::Loop, subreactors_[i]);
    thread_pool_->Add(cd);
  }

  acceptor_->EnableListening();
}

Server::~Server() = default;

void Server::NewConnection(const shared_ptr<TCPSocket> conn_socket) {
  int fd = conn_socket->GetFd();
  int idx = fd % subreactors_.size();
  auto clnt_conn = std::make_unique<Connection>(subreactors_[idx], conn_socket);
  function<void(shared_ptr<TCPSocket>)> cb = std::bind(&Server::RemoveConnection, this, std::placeholders::_1);
  clnt_conn->SetRemoveConnection(cb);
  clnt_conn->EnableReading();
  connections_[conn_socket->GetFd()] = std::move(clnt_conn);
}

void Server::RemoveConnection(const std::shared_ptr<TCPSocket> conn_socket) {
  ::close(conn_socket->GetFd());
  cout << "client fd " << conn_socket->GetFd() << " removed from connection map" << std::endl;
  connections_.erase(conn_socket->GetFd());
}

// void Server::OpenThreadPool(){
//     thread_pool.reset(new ThreadPool());
//     Errif(thread_pool == nullptr, "new threadPool error");
// }
