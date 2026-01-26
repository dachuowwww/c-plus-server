#pragma once
#include <functional>
// #include <cstdint>
#include <memory>
#include <string>
#include "Macro.h"
#include "TimeStamp.h"
class HttpContext;
class EventLoop;
class Socket;
class Channel;
class Buffer;
class Context;

class Connection : public std::enable_shared_from_this<Connection> {
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
  void SetRemoveConnection(std::function<void(const std::shared_ptr<Connection> &conn)> &&cb);
  void RemoveConnection();

  [[nodiscard]] bool IsInEpoll() const;
  [[nodiscard]] int GetFd() const;
  [[nodiscard]] std::string ReadInputBuffer() const;
  [[nodiscard]] int ReadInputBufferSize() const;
  // void ConsumeInputBuffer(int len);
  [[nodiscard]] std::string ReadOutputBuffer() const;
  [[nodiscard]] int ReadOutputBufferSize() const;
  [[nodiscard]] std::string RetriveInputBuffer() const;
  [[nodiscard]] State GetState() const;
  [[nodiscard]] EventLoop *GetLoop() const;
  [[nodiscard]] HttpContext *GetContext() const;

  // void EnableReading();
  void ConnectionEstablished();
  void ConnectionDestructor();
  void SetET();
  void SetHandleReadFunc(std::function<void(const std::shared_ptr<Connection> &conn)> cb);
  void SetConnect(std::function<void(const std::shared_ptr<Connection> &conn)> cb);

  void ListenClientMessage();
  void SendClientMessage();
  // void Echo();
  void Send(const std::string &data);

  void Send(const char *data);
  void Send(const char *data, int size);  // 一般情况最好声明长度，避免多次调用strlen
  void SendFile(int fd, int size);
  void Read();
  void Write();
  void KeyBoardToOutput();
  // void SetCloseAfterWrite();

  void SetOutput(const char *data);
  void SetState(State state);

  void UpdateTimeStamp();
  TimeStamp GetTimeStamp() const;

 private:
  EventLoop *loop_ = nullptr;
  std::unique_ptr<Socket> conn_socket_;
  std::unique_ptr<Channel> conn_channel_;
  std::function<void(const std::shared_ptr<Connection> &conn)> handle_read_func_;
  std::function<void(const std::shared_ptr<Connection> &conn)> connnect_func_;
  std::function<void(const std::shared_ptr<Connection> &conn)> remove_;
  State state_ = State::Invaild;

  std::unique_ptr<Buffer> input_buffer_;
  std::unique_ptr<Buffer> output_buffer_;

  std::unique_ptr<HttpContext> context_;
  TimeStamp conn_time_ = TimeStamp::Now();

  // bool is_http_ = true;

  void ReadBlocking();
  void WriteBlocking();
  void ReadNonBlocking();
  void WriteNonBlocking();
  void SendFileInLoop();
  // void MaybeClose();

  // bool close_after_write_ = false;
  bool sending_file_ = false;
  int sendfile_fd_ = -1;
  off_t sendfile_offset_ = 0;
  size_t sendfile_remaining_ = 0;

  DISALLOW_COPY_AND_ASSIGN(Connection);
};
