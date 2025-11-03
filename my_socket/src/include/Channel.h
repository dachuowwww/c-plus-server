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
  Channel(std::shared_ptr<EventLoop> loop, int fd);
  ~Channel();
  Channel(const Channel &) = delete;
  Channel &operator=(const Channel &) = delete;
  Channel(Channel &&) = delete;
  Channel &operator=(Channel &&) = delete;
  void SetReadCallback(const std::function<void()>& cd);
  void SetWriteCallback(const std::function<void()>& cd);
  void SetCloseCallback(const std::function<void()>& cd);

  [[nodiscard]] int GetFd() const;
  [[nodiscard]] uint32_t GetEvents() const;
  [[nodiscard]] uint32_t GetRevents() const;
  [[nodiscard]] bool IfInEpoll() const;
  // void SetThreadPool(bool use);

  void EnableReading();
  void EnableServReading();
  void DisableReading();

  void EnableWriting();
  void DisableWriting();

  void SetRevents(uint32_t revents);
  void HandleEvent();

  void RemoveInEpoll();
};
