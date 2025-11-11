#pragma once
#include <sys/epoll.h>
#include <cstdint>
#include <functional>
#include <memory>
#include "Macro.h"
class EventLoop;
class Channel {
  std::shared_ptr<EventLoop> loop_;
  int fd_ = -1;
  uint32_t events_ = EPOLLRDHUP;  // 注册的事件 EPOLLRDHUP
  uint32_t revents_ = 0;          // 实际发生的事件
  bool in_epoll_ = false;         // 是否在epoll树上
  // bool use_thread_pool_ = false;

  std::function<void()> read_call_back_;
  std::function<void()> write_call_back_;
  std::function<void()> close_call_back_;

 public:
  Channel(std::shared_ptr<EventLoop> loop, int fd);
  ~Channel() = default;
  void SetReadCallback(std::function<void()> &&cb);
  void SetWriteCallback(std::function<void()> &&cb);

  [[nodiscard]] int GetFd() const;
  [[nodiscard]] uint32_t GetEvents() const;
  [[nodiscard]] uint32_t GetRevents() const;
  [[nodiscard]] bool IfInEpoll() const;
  void SetInEpoll();
  void RemoveInEpoll();
  // void SetThreadPool(bool use);

  void EnableReading();
  void DisableReading();

  void EnableWriting();
  void DisableWriting();

  void UseET();

  void SetRevents(uint32_t revents);
  void HandleEvent();

  DISALLOW_COPY_AND_ASSIGN(Channel);
};
