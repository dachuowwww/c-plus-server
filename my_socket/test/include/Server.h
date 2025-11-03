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
class Server {
 private:
  int serve_fd_;
  std::shared_ptr<EventLoop> main_reactor_;
  std::shared_ptr<Acceptor> acceptor_;
  std::map<int, std::unique_ptr<Connection>> connections_;  // 保存所有已连接的客户端channel
  std::vector<std::shared_ptr<EventLoop>> subreactors_;     // 子Reactor
  std::unique_ptr<ThreadPool> thread_pool_;                 // 线程池，用于处理客户端请求
 public:
  explicit Server(EventLoop * loop);
  ~Server();
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;
  Server(Server &&) = delete;
  Server &operator=(Server &&) = delete;
  void NewConnection(std::shared_ptr<TCPSocket> conn_sock);
  void RemoveConnection(std::shared_ptr<TCPSocket> conn_sock);

  // void OpenThreadPool();
};
