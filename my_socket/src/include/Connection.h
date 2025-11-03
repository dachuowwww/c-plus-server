#pragma once
#include <functional>
#include <memory>
class EventLoop;
class TCPSocket;
class Channel;
class Buffer;

using std::function;
using std::shared_ptr;
using std::unique_ptr;
class Connection {
 private:
  shared_ptr<EventLoop> loop_;
  shared_ptr<TCPSocket> conn_socket_;
  unique_ptr<Channel> conn_channel_;
  function<void(shared_ptr<TCPSocket>)> remove_;
  std::atomic<bool> connected_;

  unique_ptr<Buffer> input_buffer_;
  unique_ptr<Buffer> output_buffer_;

 public:
  Connection(shared_ptr<EventLoop>, shared_ptr<TCPSocket>);
  ~Connection();
  Connection(const Connection &) = delete;
  Connection &operator=(const Connection &) = delete;
  Connection(Connection &&) = delete;
  Connection &operator=(Connection &&) = delete;
  void SetRemoveConnection(function<void(shared_ptr<TCPSocket>)> &);
  void RemoveConnection();
  bool IsInEpoll() const;
  int GetFd() const;
  void EnableReading();
  void Echo();

  bool IsConnected() const;
};
