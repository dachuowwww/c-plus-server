#pragma once
class EventLoop;
class InetAddress;
class TCPSocket;
class Channel;
#include <functional>
#include <memory>
using std::function;
using std::shared_ptr;

class Acceptor {
 private:
  shared_ptr<EventLoop> loop_;
  shared_ptr<InetAddress> listen_addr_;
  shared_ptr<TCPSocket> accept_socket_;
  std::unique_ptr<Channel> accept_channel_;
  function<void(shared_ptr<TCPSocket>)> new_connection_callback_;

 public:
  explicit Acceptor(EventLoop *);
  ~Acceptor();

  Acceptor(const Acceptor &) = delete;
  Acceptor &operator=(const Acceptor &) = delete;
  Acceptor(Acceptor &&) = delete;
  Acceptor &operator=(Acceptor &&) = delete;

  void SetNewConnectionCallback(function<void(shared_ptr<TCPSocket>)> &);
  void Accept();
  bool IsInEpoll() const;
  int GetFd() const;
  void EnableListening();
};
