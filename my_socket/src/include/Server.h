#pragma once
class EventLoop;
class Channel;
class Acceptor;
class TCPSocket;
class Connection;
class ThreadPool;
#include <map>
#include <memory>
#include <vector>
#include <functional>
class Server {
 private:
  std::shared_ptr<EventLoop> main_reactor_;  // 主Reactor
  std::shared_ptr<Acceptor> acceptor_;
  std::map<int, std::unique_ptr<Connection>> connections_;  // 保存所有已连接的客户端channel
  std::vector<std::shared_ptr<EventLoop>> subreactors_;     // 子Reactor
  std::unique_ptr<ThreadPool> thread_pool_;                 // 线程池，用于处理客户端请求
  std::function<void(Connection *)> new_connection_callback_;

 public:
  explicit Server(EventLoop *loop);
  ~Server();
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;
  Server(Server &&) = delete;
  Server &operator=(Server &&) = delete;
  void NewConnection(const std::shared_ptr<TCPSocket> &conn_sock);
  void RemoveConnection(const std::shared_ptr<TCPSocket> &conn_sock);

  void OnConnect(const std::function<void(Connection *)> &cb);
  void Handle(Connection *conn);
  // void OpenThreadPool();
};
