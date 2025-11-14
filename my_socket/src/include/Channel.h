#pragma once
#include <sys/epoll.h>
#include <cstdint>
#include <functional>
#include <memory>
#include "Macro.h"
class EventLoop;
class Channel {
 public:
  static const int READ_EVENT;
  static const int WRITE_EVENT;
  static const int ET_EVENT;

  Channel(EventLoop *loop, int fd);
  ~Channel() = default;
  [[nodiscard]] int GetFd() const;
  [[nodiscard]] uint16_t GetListenEvents() const;
  [[nodiscard]] uint16_t GetReadyEvents() const;
  [[nodiscard]] bool IfInEpoll() const;
  void SetInEpoll();
  void RemoveInEpoll();
  // void SetThreadPool(bool use);

  void EnableReading();
  void DisableReading();

  void EnableWriting();
  void DisableWriting();

  void UseET();

  void SetReadyEvents(int n);
  void SetReadCallback(std::function<void()> &&cb);
  void SetWriteCallback(std::function<void()> &&cb);
  void HandleEvent();

 private:
  EventLoop *loop_ = nullptr;
  int fd_ = -1;
  uint16_t listen_events_ = 0;  // 注册的事件 EPOLLRDHUP
  uint16_t ready_events_ = 0;   // 实际发生的事件
  bool in_epoll_ = false;       // 是否在epoll树上

  std::function<void()> read_call_back_;
  std::function<void()> write_call_back_;
  std::function<void()> close_call_back_;

  DISALLOW_COPY_AND_ASSIGN(Channel);
};
