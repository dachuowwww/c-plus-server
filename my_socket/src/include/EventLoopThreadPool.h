#pragma once
#include <memory>
#include <vector>
#include "Macro.h"
class EventLoopThread;
class EventLoop;
class EventLoopThreadPool {
 public:
  EventLoopThreadPool(EventLoop *main_reactor, int numThreads);
  ~EventLoopThreadPool();

  void SetNumThreads(int numThreads);
  void Start();
  EventLoop *NextLoop();

 private:
  EventLoop *main_reactor_;
  int num_threads_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> event_loops_;
  size_t next_;

  DISALLOW_COPY_AND_ASSIGN(EventLoopThreadPool);
};
