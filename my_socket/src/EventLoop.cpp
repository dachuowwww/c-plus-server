#include "EventLoop.h"
#include <sys/eventfd.h>
#include <iostream>
#include <mutex>
#include <vector>
#include "Channel.h"
#include "CurrentThread.h"
#include "Error.h"
#include "Poller.h"
#include "TimeQueue.h"
#include "TimeStamp.h"

EventLoop::EventLoop() {
  poller_ = std::make_unique<Poller>();
  tid_ = current_thread::Tid();
  std::cout << "EventLoop created in thread tid_=" << tid_ << std::endl;
  wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  wakeup_channel_ = std::make_unique<Channel>(this, wakeup_fd_);
  wakeup_channel_->SetReadCallback(std::bind(&EventLoop::HandleRead, this));
  wakeup_channel_->EnableReading();
  timequeue_ = std::make_unique<TimeQueue>(this);
}
EventLoop::~EventLoop() = default;
void EventLoop::Update(Channel *channel) { poller_->UpdateChannel(channel); }

void EventLoop::DoToDoList() {
  calling_functors_ = true;
  std::vector<std::function<void()>> functors;
  {
    std::unique_lock<std::mutex> lock(mtx_);
    functors.swap(to_do_list_);
  }
  for (auto &it : functors) {
    it();
  }

  calling_functors_ = false;
}

void EventLoop::Loop() {
  while (true) {
    std::vector<Channel *> chs;
    chs = poller_->Poll();  // 阻塞等待事件发生
    for (auto *it : chs) {  // more clare
      it->HandleEvent();
    }
    DoToDoList();
  }
}

void EventLoop::Delete(Channel *channel) { poller_->DeleteChannel(channel); }

bool EventLoop::IsInLoopThread() const { return current_thread::Tid() == tid_; }

void EventLoop::QueueOneFunc(std::function<void()> &&cb) {
  std::lock_guard<std::mutex> lock(mtx_);
  to_do_list_.emplace_back(cb);

  if (!(IsInLoopThread()) | calling_functors_) {
    uint64_t write_one_byte = 1;
    ssize_t write_size = write(wakeup_fd_, &write_one_byte, sizeof(write_one_byte));

    Errif(write_size != sizeof(write_one_byte), "Wake up write error.");
    (void)write_size;
  }
}

void EventLoop::RunOneFunc(std::function<void()> &&cb) {
  if (IsInLoopThread()) {
    cb();
  } else {
    QueueOneFunc(std::move(cb));
  }
}

void EventLoop::HandleRead() {
  uint64_t read_one_byte = 1;
  ssize_t read_size = read(wakeup_fd_, &read_one_byte, sizeof(read_one_byte));

  Errif(read_size != sizeof(read_one_byte), "Wake up read error.");
  (void)read_size;
}

void EventLoop::RunAt(TimeStamp timestamp, std::function<void()> &&cb) {
  timequeue_->Insert(timestamp, std::move(cb), 0.0);
}

void EventLoop::RunAfter(double wait_time, std::function<void()> &&cb) {
  timequeue_->Insert(TimeStamp::AddTime(TimeStamp::Now(), wait_time), std::move(cb), 0.0);
}

void EventLoop::RunEvery(double interval, std::function<void()> &&cb) {
  timequeue_->Insert(TimeStamp::AddTime(TimeStamp::Now(), interval), std::move(cb), interval);  // 不能以目前为定时器
}
