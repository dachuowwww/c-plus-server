#pragma once
class EventLoop;
class InetAddress;
class Socket;
class Channel;
#include <functional>
#include <memory>
#include "Macro.h"

class Acceptor {
 private:
  std::shared_ptr<EventLoop> loop_;
  std::shared_ptr<InetAddress> listen_addr_;
  std::shared_ptr<Socket> accept_socket_;
  std::unique_ptr<Channel> accept_channel_;
  std::function<void(std::shared_ptr<Socket> &)> new_connection_callback_;

 public:
  explicit Acceptor(EventLoop *loop);
  ~Acceptor();

  void SetNewConnectionCallback(const std::function<void(std::shared_ptr<Socket> &)> &cb);
  void Accept();
  [[nodiscard]] bool IsInEpoll() const;
  [[nodiscard]] int GetFd() const;
  void EnableListening();

  DISALLOW_COPY_AND_ASSIGN(Acceptor);
};
