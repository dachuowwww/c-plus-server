#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include "Macro.h"
class EventLoop;

class EventLoopThread {
 public:
  EventLoopThread();
  ~EventLoopThread();
  EventLoop *StartLoop();
  void ThreadFunc();

 private:
  std::jthread thread_;
  std::unique_ptr<EventLoop> loop_;
  std::mutex mtx_;
  std::condition_variable cv_;

  DISALLOW_COPY_AND_ASSIGN(EventLoopThread);
};
