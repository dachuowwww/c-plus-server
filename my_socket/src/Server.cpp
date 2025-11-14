#include "Server.h"
#include <unistd.h>
#include <iostream>
#include "Acceptor.h"
#include "Channel.h"
#include "Connection.h"
#include "Error.h"
#include "EventLoop.h"
#include "ThreadPool.h"

using std::cout;
using std::endl;
using std::function;
Server::Server(EventLoop *loop) : main_reactor_(loop) {
  acceptor_ = std::make_unique<Acceptor>(main_reactor_);
  acceptor_->SetNewConnectionCallback([this](int cln_fd) { Server::NewConnection(cln_fd); });

  unsigned int size = std::thread::hardware_concurrency();
  thread_pool_ = std::make_unique<ThreadPool>(size);
  subreactors_.reserve(size);
  for (unsigned int i = 0; i < size; ++i) {
    subreactors_.emplace_back(std::make_unique<EventLoop>());
    Errif(subreactors_[i] == nullptr, "subReactors emplace_back error");
    function<void()> cb = std::bind(&EventLoop::Loop, subreactors_[i].get());
    thread_pool_->Add(std::move(cb));
  }

  acceptor_->EnableListening();
}
Server::~Server() = default;

void Server::OnConnect(function<void(Connection *)> &&cb) { new_connection_callback_ = std::move(cb); }

void Server::NewConnection(int cln_fd) {
  int idx = static_cast<int>(cln_fd % subreactors_.size());
  auto clnt_conn = std::make_unique<Connection>(subreactors_[idx].get(), cln_fd);
  // function<void(Connection *)> cb1 = std::bind(&Handle, this, std::placeholders::_1);
  // 没有必要，因为Server::Handle已经绑定了this指针
  clnt_conn->SetHandleReadFunc(new_connection_callback_);
  clnt_conn->SetRemoveConnection([this](int cln_fd) { Server::RemoveConnection(cln_fd); });
  clnt_conn->SetET();
  clnt_conn->EnableReading();
  connections_[cln_fd] = std::move(clnt_conn);
}

void Server::RemoveConnection(int cln_fd) {
  if (connections_.find(cln_fd) == connections_.end()) {
    cout << "client fd " << cln_fd << " not found in connection map" << std::endl;
    return;
  }
  connections_.erase(cln_fd);
  cout << "client fd " << cln_fd << " removed from connection map" << std::endl;
}

// void Server::OpenThreadPool(){
//     thread_pool.reset(new ThreadPool());
//     Errif(thread_pool == nullptr, "new threadPool error");
// }
