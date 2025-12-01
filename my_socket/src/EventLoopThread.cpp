#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread() : loop_(nullptr) {}

EventLoopThread::~EventLoopThread() = default;

EventLoop *EventLoopThread::StartLoop() {  // 同步问题
  thread_ = std::jthread([this] { ThreadFunc(); });
  {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] { return loop_ != nullptr; });
    return loop_.get();
  }
}

void EventLoopThread::ThreadFunc() {
  auto loop = std::make_unique<EventLoop>();
  {
    std::unique_lock<std::mutex> lock(mtx_);
    loop_ = std::move(loop);
  }
  cv_.notify_one();
  loop_->Loop();
}
