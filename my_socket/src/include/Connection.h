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
  std::function<void(Connection *)> handle_read_func_;
  std::function<void(std::shared_ptr<TCPSocket> &)> remove_;
  std::atomic<bool> connected_ = true;

  std::unique_ptr<Buffer> input_buffer_;
  std::unique_ptr<Buffer> output_buffer_;
  void ReadBlocking();
  void WriteBlocking();
  void ReadNonBlocking();
  void WriteNonBlocking();

 public:
  Connection(std::shared_ptr<EventLoop> loop, std::shared_ptr<TCPSocket> conn_socket);
  ~Connection();
  Connection(const Connection &) = delete;
  Connection &operator=(const Connection &) = delete;
  Connection(Connection &&) = delete;
  Connection &operator=(Connection &&) = delete;
  void SetRemoveConnection(const std::function<void(std::shared_ptr<TCPSocket> &)> &cb);
  void RemoveConnection();
  void Close();
  [[nodiscard]] bool IsInEpoll() const;
  [[nodiscard]] int GetFd() const;
  void EnableReading();
  void SetHandleReadFunc(const std::function<void(Connection *)> &cb);
  void HandleRead();
  // void Echo();

  void Read();
  void Write();

  void ReadKeyBoard();
  void SetOutput(const char *data);

  [[nodiscard]] const char *ReadBuffer() const;
  [[nodiscard]] bool IsConnected() const;
};
