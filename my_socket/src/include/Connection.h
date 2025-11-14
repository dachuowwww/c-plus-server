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

  Connection(EventLoop *loop, int cln_fd);
  ~Connection();
  void SetRemoveConnection(std::function<void(int)> &&cb);
  void RemoveConnection();

  [[nodiscard]] bool IsInEpoll() const;
  [[nodiscard]] int GetFd() const;
  [[nodiscard]] const char *ReadInputBuffer() const;
  [[nodiscard]] State GetState() const;

  void EnableReading();
  void SetET();
  void SetHandleReadFunc(std::function<void(Connection *)> cb);

  void ListenClientMessage();
  // void Echo();
  void Send(const char *data);
  void Read();
  void Write();
  void KeyBoardInput();

  void SetOutput(const char *data);
  void SetState(State state);

 private:
  EventLoop *loop_ = nullptr;
  std::unique_ptr<Socket> conn_socket_;
  std::unique_ptr<Channel> conn_channel_;
  std::function<void(Connection *)> handle_read_func_;
  std::function<void(int)> remove_;
  State state_ = State::Invaild;

  std::unique_ptr<Buffer> input_buffer_;
  std::unique_ptr<Buffer> output_buffer_;
  void ReadBlocking();
  void WriteBlocking();
  void ReadNonBlocking();
  void WriteNonBlocking();

  DISALLOW_COPY_AND_ASSIGN(Connection);
};
