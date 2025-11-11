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

Channel::Channel(shared_ptr<EventLoop> loop, int fd) : loop_(std::move(loop)), fd_(fd) {}

void Channel::SetReadCallback(function<void()> &&cb) { read_call_back_ = std::move(cb); }
void Channel::SetWriteCallback(function<void()> &&cb) { write_call_back_ = std::move(cb); }

int Channel::GetFd() const { return fd_; }
uint32_t Channel::GetEvents() const { return events_; }
uint32_t Channel::GetRevents() const { return revents_; }

bool Channel::IfInEpoll() const { return in_epoll_; }

// void Channel::SetThreadPool(bool use){
//     useThreadPool = use;
// }
void Channel::SetInEpoll() { in_epoll_ = true; }
void Channel::RemoveInEpoll() {
  if (in_epoll_) {
    loop_->Delete(this);
    events_ = EPOLLRDHUP;
    revents_ = 0;
    in_epoll_ = false;
  } else {
    cout << "Channel::RemoveInEpoll error" << endl;
  }
}

void Channel::EnableReading() {
  events_ |= EPOLLIN;

  loop_->Update(this);
}
void Channel::DisableReading() {
  events_ &= ~EPOLLIN;
  if (events_ == EPOLLRDHUP) {
    loop_->Delete(this);
  } else {
    loop_->Update(this);
  }
}

void Channel::EnableWriting() {
  events_ |= EPOLLOUT;
  loop_->Update(this);
}
void Channel::DisableWriting() {
  events_ &= ~EPOLLOUT;
  if (events_ == EPOLLRDHUP) {
    loop_->Delete(this);
  } else {
    loop_->Update(this);
  }
}

void Channel::UseET() {
  events_ |= EPOLLET;
  loop_->Update(this);
}

void Channel::SetRevents(uint32_t revents) { revents_ = revents; }
void Channel::HandleEvent() {
  if ((revents_ & EPOLLIN) && read_call_back_) {  // 客户端退出连接也会读取
    read_call_back_();
  }
  if ((revents_ & EPOLLOUT) && write_call_back_) {
    write_call_back_();
  }
}
