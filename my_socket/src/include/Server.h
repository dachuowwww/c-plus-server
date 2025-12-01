#pragma once
class EventLoop;
class Acceptor;
// class Socket;
class Connection;
class ThreadPool;
class EventLoopThreadPool;
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Macro.h"
class Server {
 public:
  explicit Server(EventLoop *loop);
  ~Server();
  void NewConnection(int cln_fd);
  void RemoveConnection(const std::shared_ptr<Connection> &conn);
  void RemoveConnectionInLoop(const std::shared_ptr<Connection> &conn);

  void OnConnect(std::function<void(const std::shared_ptr<Connection> &conn)> &&cb);

 private:
  EventLoop *main_reactor_;  // 主Reactor,在acceptor中监听连接请求
  std::unique_ptr<Acceptor> acceptor_;
  std::unordered_map<int, std::shared_ptr<Connection>> connections_;  // 保存所有已连接的客户端channel
  // std::vector<std::unique_ptr<EventLoop>> subreactors_;               // 子Reactor
  // std::unique_ptr<ThreadPool> thread_pool_;                           // 线程池，用于处理客户端请求
  std::unique_ptr<EventLoopThreadPool> event_loop_thread_pool_;
  std::function<void(const std::shared_ptr<Connection> &conn)> new_connection_callback_;

  DISALLOW_COPY_AND_ASSIGN(Server);
};
