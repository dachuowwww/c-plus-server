#include "Channel.h"
#include <unistd.h>
#include <iostream>
#include "EventLoop.h"
#include "Server.h"
using std::cout;
using std::endl;
using std::function;
using std::make_shared;
using std::shared_ptr;
const int Channel::READ_EVENT = 1;
const int Channel::WRITE_EVENT = 2;
const int Channel::ET_EVENT = 4;

Channel::Channel(shared_ptr<EventLoop> loop, int fd) : loop_(std::move(loop)), fd_(fd) {}

void Channel::SetReadCallback(function<void()> &&cb) { read_call_back_ = std::move(cb); }
void Channel::SetWriteCallback(function<void()> &&cb) { write_call_back_ = std::move(cb); }

int Channel::GetFd() const { return fd_; }
uint32_t Channel::GetListenEvents() const { return listen_events_; }
uint32_t Channel::GetReadyEvents() const { return ready_events_; }

bool Channel::IfInEpoll() const { return in_epoll_; }

// void Channel::SetThreadPool(bool use) {
//     useThreadPool = use;
// }

void Channel::SetInEpoll() { in_epoll_ = true; }
void Channel::RemoveInEpoll() {
  if (in_epoll_) {
    loop_->Delete(this);
    listen_events_ = 0;
    ready_events_ = 0;
    in_epoll_ = false;
  } else {
    cout << "Channel::RemoveInEpoll error" << endl;
  }
}

void Channel::EnableReading() {
  listen_events_ |= READ_EVENT;

  loop_->Update(this);
}
void Channel::DisableReading() {
  listen_events_ &= ~READ_EVENT;
  if (listen_events_ == 0) {
    loop_->Delete(this);
  } else {
    loop_->Update(this);
  }
}

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
  if ((ready_events_ & READ_EVENT) && read_call_back_) {  // 客户端退出连接也会读取
    read_call_back_();
  }
  if ((ready_events_ & WRITE_EVENT) && write_call_back_) {
    write_call_back_();
  }
}
