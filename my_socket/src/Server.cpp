#include "Server.h"
#include <unistd.h>
#include <iostream>
#include <thread>
#include "Acceptor.h"
// #include "Channel.h"
#include "Connection.h"
#include "Error.h"
#include "EventLoop.h"
#include "Metrics.h"
// #include "ThreadPool.h"
#include "EventLoopThreadPool.h"
#include "Logger.h"

using std::cout;
using std::endl;
using std::function;
Server::Server(EventLoop *loop, const char *ip, uint16_t port) : main_reactor_(loop) {
  acceptor_ = std::make_unique<Acceptor>(main_reactor_, ip, port);
  acceptor_->SetNewConnectionCallback([this](int cln_fd) { Server::NewConnection(cln_fd); });

  unsigned int size = std::thread::hardware_concurrency();
  event_loop_thread_pool_ = std::make_unique<EventLoopThreadPool>(main_reactor_, size);
  event_loop_thread_pool_->Start();
  // thread_pool_ = std::make_unique<ThreadPool>(size);
  // subreactors_.reserve(size);
  // for (unsigned int i = 0; i < size; ++i) {
  //   subreactors_.emplace_back(std::make_unique<EventLoop>());
  //   Errif(subreactors_[i] == nullptr, "subReactors emplace_back error");
  //   function<void()> cb = std::bind(&EventLoop::Loop, subreactors_[i].get());
  //   thread_pool_->Add(std::move(cb));
  // }

  acceptor_->EnableListening();
}
Server::~Server() = default;

void Server::OnMessage(function<void(const std::shared_ptr<Connection> &conn)> &&cb) {
  message_callback_ = std::move(cb);
}

void Server::OnConnect(function<void(const std::shared_ptr<Connection> &conn)> &&cb) {
  connect_callback_ = std::move(cb);
}

void Server::NewConnection(int cln_fd) {
  // int idx = static_cast<int>(cln_fd % subreactors_.size());
  Metrics::OnAccept();
  auto clnt_conn = std::make_shared<Connection>(event_loop_thread_pool_->NextLoop(), cln_fd);
  // function<void(Connection *)> cb1 = std::bind(&Handle, this, std::placeholders::_1);
  // 没有必要，因为Server::Handle已经绑定了this指针
  clnt_conn->SetConnect(connect_callback_);
  clnt_conn->SetHandleReadFunc(message_callback_);
  clnt_conn->SetRemoveConnection([this](const std::shared_ptr<Connection> &conn) { Server::RemoveConnection(conn); });
  clnt_conn->SetET();
  // clnt_conn->EnableReading();
  connections_[cln_fd] = clnt_conn;  // 线程不安全  +1
  clnt_conn->ConnectionEstablished();
}

void Server::RemoveConnection(const std::shared_ptr<Connection> &conn) {
  // LOG_INFO << "Server::HandleCloseInLoop - RemoveConnection [fd#" << conn->GetFd() << "]";

  Errif(connections_.find(conn->GetFd()) == connections_.end(), "client fd not found in connection map");
  main_reactor_->RunOneFunc(std::bind(&Server::RemoveConnectionInLoop, this, conn));
}

void Server::RemoveConnectionInLoop(const std::shared_ptr<Connection> &conn) {
  if (connections_.erase(conn->GetFd()) > 0) {  // -1
    Metrics::OnClose();
  }
  // LOG_INFO << "Server::RemoveConnectionInLoop - Client fd " << conn->GetFd() << " removed from connection map";

  EventLoop *loop = conn->GetLoop();
  loop->QueueOneFunc(std::bind(&Connection::ConnectionDestructor, conn));
}

// void Server::OpenThreadPool(){
//     thread_pool.reset(new ThreadPool());
//     Errif(thread_pool == nullptr, "new threadPool error");
// }
