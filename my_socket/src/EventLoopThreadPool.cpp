#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *main_reactor, int numThreads)
    : main_reactor_(main_reactor), num_threads_(numThreads), next_(0) {
  if (num_threads_ <= 0) {
    num_threads_ = 1;
  }
}

EventLoopThreadPool::~EventLoopThreadPool() = default;

void EventLoopThreadPool::SetNumThreads(int numThreads) { num_threads_ = numThreads; }

void EventLoopThreadPool::Start() {
  for (int i = 0; i < num_threads_; ++i) {
    threads_.emplace_back(std::make_unique<EventLoopThread>());
    event_loops_.emplace_back(threads_.back()->StartLoop());
  }
}

EventLoop *EventLoopThreadPool::NextLoop() {
  if (!event_loops_.empty()) {
    if (next_ == event_loops_.size()) {
      next_ = 0;
    }
    return event_loops_[next_++];
  }
  return main_reactor_;
}
