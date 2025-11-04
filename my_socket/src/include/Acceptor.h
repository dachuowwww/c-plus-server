#pragma once
class EventLoop;
class InetAddress;
class TCPSocket;
class Channel;
#include <functional>
#include <memory>

class Acceptor {
 private:
  std::shared_ptr<EventLoop> loop_;
  std::shared_ptr<InetAddress> listen_addr_;
  std::shared_ptr<TCPSocket> accept_socket_;
  std::unique_ptr<Channel> accept_channel_;
  std::function<void(std::shared_ptr<TCPSocket> &)> new_connection_callback_;

 public:
  explicit Acceptor(EventLoop *loop);
  ~Acceptor();

  Acceptor(const Acceptor &) = delete;
  Acceptor &operator=(const Acceptor &) = delete;
  Acceptor(Acceptor &&) = delete;
  Acceptor &operator=(Acceptor &&) = delete;

  void SetNewConnectionCallback(const std::function<void(std::shared_ptr<TCPSocket> &)> &cb);
  void Accept();
  [[nodiscard]] bool IsInEpoll() const;
  [[nodiscard]] int GetFd() const;
  void EnableListening();
};
