#pragma once
#include <functional>
#include <memory>
#include "Macro.h"
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
  void Update(Channel *channel);
  void Loop();
  void Delete(Channel *channel);
  DISALLOW_COPY_AND_ASSIGN(EventLoop);
};
