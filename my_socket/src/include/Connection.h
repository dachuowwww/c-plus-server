#pragma once
#include <atomic>
#include <functional>
#include <memory>
class EventLoop;
class TCPSocket;
class Channel;
class Buffer;

class Connection {
 private:
  std::shared_ptr<EventLoop> loop_;
  std::shared_ptr<TCPSocket> conn_socket_;
  std::unique_ptr<Channel> conn_channel_;
  std::function<void(std::shared_ptr<TCPSocket> &)> remove_;
  std::atomic<bool> connected_ = true;

  std::unique_ptr<Buffer> input_buffer_;
  std::unique_ptr<Buffer> output_buffer_;

 public:
  Connection(std::shared_ptr<EventLoop> loop, std::shared_ptr<TCPSocket> conn_socket);
  ~Connection();
  Connection(const Connection &) = delete;
  Connection &operator=(const Connection &) = delete;
  Connection(Connection &&) = delete;
  Connection &operator=(Connection &&) = delete;
  void SetRemoveConnection(const std::function<void(std::shared_ptr<TCPSocket> &)> &cb);
  void RemoveConnection();
  [[nodiscard]] bool IsInEpoll() const;
  [[nodiscard]] int GetFd() const;
  void EnableReading();
  void Echo();

  [[nodiscard]] bool IsConnected() const;
};
