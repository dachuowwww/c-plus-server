#pragma once
#include <functional>
#include <memory>
#include <set>
#include <vector>
#include "Macro.h"
#include "utility"
class TimeStamp;
class Timer;
class EventLoop;
class Channel;
class TimeQueue {
 public:
  explicit TimeQueue(EventLoop *loop);
  ~TimeQueue();
  void CreateTimerfd();
  void ReadTimerfd();
  void Insert(TimeStamp timestamp, std::function<void()> &&callback, double interval);
  [[discard]] bool InsertTimer(const std::shared_ptr<Timer> &timer);
  void ResetTimers();
  void ResetTimerfd(const TimeStamp &new_time);
  void HandleTimers();

 private:
  EventLoop *loop_ = nullptr;
  int timerfd_ = -1;
  std::unique_ptr<Channel> timer_channel_;
  typedef std::pair<TimeStamp, std::shared_ptr<Timer>> Entry;
  std::set<Entry> timers_;
  std::vector<Entry> active_timers_;

  DISALLOW_COPY_AND_ASSIGN(TimeQueue);
};
