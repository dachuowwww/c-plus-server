#pragma once
#include <functional>
#include <memory>
class Channel;
class Epoll;
class ThreadPool;
class EventLoop {
 private:
  /* data */
  std::unique_ptr<Epoll> ep_;

 public:
  EventLoop();
  ~EventLoop();
  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;
  EventLoop(EventLoop &&) = delete;
  EventLoop &operator=(EventLoop &&) = delete;
  void Update(Channel *);
  void Loop();
  void Delete(Channel *);
};
