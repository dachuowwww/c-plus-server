#pragma once
class EventLoop;
class Channel;
class Acceptor;
class Socket;
class Connection;
class ThreadPool;
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include "Macro.h"
class Server {
 private:
  std::shared_ptr<EventLoop> main_reactor_;  // 主Reactor,在acceptor中监听连接请求
  std::shared_ptr<Acceptor> acceptor_;
  std::map<int, std::unique_ptr<Connection>> connections_;  // 保存所有已连接的客户端channel
  std::vector<std::shared_ptr<EventLoop>> subreactors_;     // 子Reactor
  std::unique_ptr<ThreadPool> thread_pool_;                 // 线程池，用于处理客户端请求
  std::function<void(Connection *)> new_connection_callback_;

 public:
  explicit Server(std::shared_ptr<EventLoop> loop);
  ~Server();
  void NewConnection(const std::shared_ptr<Socket> &conn_sock);
  void RemoveConnection(const std::shared_ptr<Socket> &conn_sock);

  void OnConnect(std::function<void(Connection *)> &&cb);
  // void OpenThreadPool();
  DISALLOW_COPY_AND_ASSIGN(Server);
};
