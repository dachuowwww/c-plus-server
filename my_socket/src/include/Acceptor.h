#pragma once
class EventLoop;
class Socket;
class Channel;
#include <functional>
#include <memory>
#include "Macro.h"

class Acceptor {
 public:
  explicit Acceptor(EventLoop *loop);
  ~Acceptor();

  void SetNewConnectionCallback(std::function<void(int)> &&cb);
  void Accept();
  [[nodiscard]] bool IsInEpoll() const;
  [[nodiscard]] int GetFd() const;
  void EnableListening();

 private:
  EventLoop *loop_ = nullptr;  // 类成员用指针写可以不引用完整类型
  std::unique_ptr<Socket> accept_socket_;
  std::unique_ptr<Channel> accept_channel_;  // unique_ptr需要完整声明内部类型才能在本定义内使用默认析构函数
  std::function<void(int)> new_connection_callback_;

  DISALLOW_COPY_AND_ASSIGN(Acceptor);
};
