#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "Macro.h"
class Channel;
class Poller;
class TimeQueue;
class TimeStamp;
class EventLoop {
 public:
  EventLoop();
  ~EventLoop();
  void Update(Channel *channel);
  void Loop();
  void Delete(Channel *channel);

  [[nodiscard]] bool IsInLoopThread() const;
  void DoToDoList();
  void RunOneFunc(std::function<void()> &&cb);
  void QueueOneFunc(std::function<void()> &&cb);

  void HandleRead();

  void RunAt(TimeStamp timestamp, std::function<void()> &&cb);
  void RunAfter(double wait_time, std::function<void()> &&cb);
  void RunEvery(double interval, std::function<void()> &&cb);

 private:
  std::unique_ptr<Poller> poller_;
  std::vector<std::function<void()>> to_do_list_;
  std::mutex mtx_;
  pid_t tid_ = 0;  // linux thread id data describer
  int wakeup_fd_;
  std::unique_ptr<Channel> wakeup_channel_;
  bool calling_functors_ = false;  // 记录是否在调用dotodolist判断需不需要唤醒

  std::unique_ptr<TimeQueue> timequeue_;

  DISALLOW_COPY_AND_ASSIGN(EventLoop);
};
