#include "Channel.h"
#include <iostream>
#include "Connection.h"
#include "Error.h"
#include "EventLoop.h"
#include "Logger.h"
using std::cout;
using std::endl;
using std::function;
const int Channel::READ_EVENT = 1;
const int Channel::WRITE_EVENT = 2;
const int Channel::ET_EVENT = 4;

Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd) {}

void Channel::SetReadCallback(function<void()> &&cb) { read_call_back_ = std::move(cb); }
void Channel::SetWriteCallback(function<void()> &&cb) { write_call_back_ = std::move(cb); }

int Channel::GetFd() const { return fd_; }

uint16_t Channel::GetListenEvents() const { return listen_events_; }

uint16_t Channel::GetReadyEvents() const { return ready_events_; }

bool Channel::IfInEpoll() const { return in_epoll_; }

// void Channel::SetThreadPool(bool use) {
//     useThreadPool = use;
// }

void Channel::SetInEpoll() { in_epoll_ = true; }

void Channel::RemoveInEpoll() {
  if (in_epoll_) {
    loop_->Delete(this);  // 线程安全
    listen_events_ = 0;
    ready_events_ = 0;
    in_epoll_ = false;
  } else {
    Errif(true, "Channel::RemoveInEpoll called when channel not in epoll");
  }
}

void Channel::EnableReading() {
  listen_events_ |= READ_EVENT;

  loop_->Update(this);
}
// void Channel::DisableReading() {
//   listen_events_ &= ~READ_EVENT;
//   if (listen_events_ == 0) {
//     loop_->Delete(this);
//   } else {
//     loop_->Update(this);
//   }
// }

void Channel::EnableWriting() {
  listen_events_ |= WRITE_EVENT;
  loop_->Update(this);
}

void Channel::DisableWriting() {
  listen_events_ &= ~WRITE_EVENT;
  if (listen_events_ == 0) {
    loop_->Delete(this);
  } else {
    loop_->Update(this);
  }
}

void Channel::UseET() {
  listen_events_ |= ET_EVENT;
  loop_->Update(this);
}

void Channel::SetReadyEvents(int n) {
  if (n == 0) {
    ready_events_ = 0;
  }
  if (n & READ_EVENT) {
    ready_events_ |= READ_EVENT;
  }
  if (n & WRITE_EVENT) {
    ready_events_ |= WRITE_EVENT;
  }
}
void Channel::HandleEvent() {
  if (tied_) {
    auto guard = tie_.lock();  // +1
    HandleEventWithGuard();
  } else {
    HandleEventWithGuard();
  }
}  // -1

void Channel::HandleEventWithGuard() {
  if ((ready_events_ & READ_EVENT) && read_call_back_) {  // 客户端退出连接也会读取
    read_call_back_();
  }
  if ((ready_events_ & WRITE_EVENT) && write_call_back_) {
    write_call_back_();
  }
}

void Channel::Tie(const std::shared_ptr<Connection> &conn) {
  tie_ = conn;
  tied_ = true;
}
