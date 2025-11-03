#pragma once
#include <cstdint>
#include <functional>
#include <memory>
class EventLoop;
class Channel {
  std::shared_ptr<EventLoop> loop_;
  const int fd_;
  uint32_t events_;   // 注册的事件
  uint32_t revents_;  // 实际发生的事件
  bool in_epoll_;     // 是否在epoll树上
  // bool use_thread_pool_;

  std::function<void()> read_call_back_;
  std::function<void()> write_call_back_;
  std::function<void()> close_call_back_;

 public:
  Channel(std::shared_ptr<EventLoop>, int);
  ~Channel();
  Channel(const Channel &) = delete;
  Channel &operator=(const Channel &) = delete;
  Channel(Channel &&) = delete;
  Channel &operator=(Channel &&) = delete;
  void SetReadCallback(std::function<void()> &);
  void SetWriteCallback(std::function<void()> &);
  void SetCloseCallback(std::function<void()> &);

  int GetFd() const;
  uint32_t GetEvents() const;
  uint32_t GetRevents() const;
  bool IfInEpoll() const;
  // void SetThreadPool(bool use);

  void EnableReading();
  void EnableServReading();
  void DisableReading();

  void EnableWriting();
  void DisableWriting();

  void SetRevents(uint32_t);
  void HandleEvent();

  void RemoveInEpoll();
};
