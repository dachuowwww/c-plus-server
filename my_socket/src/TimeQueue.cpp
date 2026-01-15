#include "TimeQueue.h"
#include <iostream>
#include "Channel.h"
#include "Error.h"
#include "EventLoop.h"
#include "TimeStamp.h"
#include "Timer.h"
#include "sys/timerfd.h"

TimeQueue::TimeQueue(EventLoop *loop) : loop_(loop) {
  CreateTimerfd();
  timer_channel_ = std::make_unique<Channel>(loop_, timerfd_);
  timer_channel_->SetReadCallback(std::bind(&TimeQueue::HandleTimers, this));
  timer_channel_->EnableReading();
}

TimeQueue::~TimeQueue() { close(timerfd_); }

void TimeQueue::CreateTimerfd() {
  timerfd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);  // uint64_t
  Errif(timerfd_ < 0, "Create timerfd failed.");
}

void TimeQueue::ReadTimerfd() {
  uint64_t read_bytes = 0;
  ssize_t readn = read(timerfd_, &read_bytes, sizeof(read_bytes));
  Errif(readn != sizeof(read_bytes), "Read timerfd failed.");
}

void TimeQueue::Insert(TimeStamp timestamp, std::function<void()> &&callback, double interval) {
  auto timer = std::make_shared<Timer>(timestamp, std::move(callback), interval);
  if (InsertTimer(timer)) {
    ResetTimerfd(timers_.begin()->first);
  }
}
bool TimeQueue::InsertTimer(const std::shared_ptr<Timer> &timer) {
  bool ifreset = false;
  if (timers_.empty() || timer->GetTimeStamp() < timers_.begin()->first) {
    ifreset = true;
  }
  timers_.emplace(Entry(timer->GetTimeStamp(), timer));
  return ifreset;
}
void TimeQueue::ResetTimers() {
  for (auto &it : active_timers_) {
    if (it.second->IfRepeat()) {
      it.second->Restart();
      InsertTimer(it.second);
    }
  }
  if (!timers_.empty()) {
    ResetTimerfd(timers_.begin()->first);  // 每次都要重置
  }
}

void TimeQueue::ResetTimerfd(const TimeStamp &new_time) {
  // std::cout<<"ResetTimerfd"<<std::endl;
  struct itimerspec new_value {};
  struct itimerspec old_value {};
  int64_t dif = new_time.Time() - TimeStamp::Now().Time();
  if (dif < 100) {
    dif = 100;
  }
  // std::cout<<dif<<std::endl;
  new_value.it_value.tv_sec = static_cast<time_t>(dif / MICROSECOND_2_SECOND);
  new_value.it_value.tv_nsec = static_cast<int64_t>((dif % MICROSECOND_2_SECOND) * 1000);
  // std::cout<<new_value.it_value.tv_sec<<std::endl;
  // std::cout<<new_value.it_value.tv_nsec<<std::endl;
  int ret = timerfd_settime(timerfd_, 0, &new_value, &old_value);
  Errif(ret < 0, "Reset timerfd failed.");
}

void TimeQueue::HandleTimers() {
  ReadTimerfd();
  active_timers_.clear();
  auto end = timers_.lower_bound(Entry(TimeStamp::Now(), std::make_shared<Timer>(TimeStamp::Now(), nullptr, 0)));
  active_timers_.insert(active_timers_.end(), timers_.begin(), end);
  for (auto &it : active_timers_) {
    it.second->Run();
  }
  timers_.erase(timers_.begin(), end);
  ResetTimers();
}
