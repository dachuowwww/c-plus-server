#pragma once
#include <functional>
#include <memory>
#include "Macro.h"
class EventLoop;
class Socket;
class Channel;
class Buffer;

class Connection {
 public:
  enum class State {
    Invaild = 1,
    Closed,
    Connected,
    Handshaking,
    Failed,
  };

  Connection(std::shared_ptr<EventLoop> loop, std::shared_ptr<Socket> conn_socket);
  ~Connection();
  void SetRemoveConnection(std::function<void(std::shared_ptr<Socket> &)> cb);
  void RemoveConnection();
  [[nodiscard]] bool IsInEpoll() const;
  [[nodiscard]] int GetFd() const;
  void EnableReading();
  void SetHandleReadFunc(std::function<void(Connection *)> cb);
  void SetET();
  // void Echo();

  void Read();
  void Write();

  void KeyBoardInput();
  void SetOutput(const char *data);

  void SetState(State state);

  [[nodiscard]] const char *ReadInputBuffer() const;
  [[nodiscard]] State GetState() const;

 private:
  std::shared_ptr<EventLoop> loop_;
  std::shared_ptr<Socket> conn_socket_;
  std::unique_ptr<Channel> conn_channel_;
  std::function<void(Connection *)> handle_read_func_;
  std::function<void(std::shared_ptr<Socket> &)> remove_;
  State state_ = State::Invaild;

  std::unique_ptr<Buffer> input_buffer_;
  std::unique_ptr<Buffer> output_buffer_;
  void ReadBlocking();
  void WriteBlocking();
  void ReadNonBlocking();
  void WriteNonBlocking();

  DISALLOW_COPY_AND_ASSIGN(Connection);
};
