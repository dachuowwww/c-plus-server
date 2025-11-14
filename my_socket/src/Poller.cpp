#include "Poller.h"
#include <cstring>
#include <iostream>
#include "Channel.h"
#include "Error.h"

#ifdef OS_LINUX
const int MAXEVENTS = 1024;
Poller::Poller() {
  poller_fd_ = epoll_create1(0);
  Errif(poller_fd_ == -1, "epoll create error");
  events_ = new epoll_event[MAXEVENTS];
  Errif(events_ == nullptr, "epoll events new error");
  memset(events_, 0, sizeof(*events_) * MAXEVENTS);
}

Poller::~Poller() {
  ::close(poller_fd_);
  delete[] events_;
}

std::vector<Channel *> Poller::Poll(int timeout) {
  // std::cout << "begin:EventLoop::Loop() epoll_fd_ = " << epoll_fd_ << std::endl;
  std::vector<Channel *> active_channels;
  // std::cout << "begin:EventLoop::Loop() events_ = " << events_ << std::endl;
  int nfds = epoll_wait(poller_fd_, events_, MAXEVENTS, timeout);
  for (int i = 0; i < nfds; ++i) {
    // std::cout << "accept:EventLoop::Loop() epoll_fd_"<< epoll_fd_ << " , events_[" << events_[i].events <<
    // "].data.ptr = " << events_[i].data.ptr << std::endl;
    Channel *ch = static_cast<Channel *>(events_[i].data.ptr);
    ch->SetReadyEvents(0);
    if (events_[i].events & EPOLLIN) {
      ch->SetReadyEvents(Channel::READ_EVENT);
    }
    if (events_[i].events & EPOLLOUT) {
      ch->SetReadyEvents(Channel::WRITE_EVENT);
    }
    active_channels.push_back(ch);
  }
  return active_channels;
}

void Poller::UpdateChannel(Channel *channel) {
  struct epoll_event ev = {};
  ev.data.ptr = channel;
  if (channel->GetListenEvents() & Channel::READ_EVENT) {
    ev.events |= EPOLLIN;
  }
  if (channel->GetListenEvents() & Channel::WRITE_EVENT) {
    ev.events |= EPOLLOUT;
  }
  if (channel->GetListenEvents() & Channel::ET_EVENT) {
    ev.events |= EPOLLET;
  }
  if (!channel->IfInEpoll()) {
    Errif(epoll_ctl(poller_fd_, EPOLL_CTL_ADD, channel->GetFd(), &ev) == -1, "epoll add error");
    channel->SetInEpoll();
  } else {
    Errif(epoll_ctl(poller_fd_, EPOLL_CTL_MOD, channel->GetFd(), &ev) == -1, "epoll modify error");
  }
  // std::cout << "Registered " << channel->GetFd() << " in epoll:" << poller_fd_<< " , event: " <<
  // channel->GetListenEvents() << std::endl;
}

void Poller::DeleteChannel(Channel *channel) {
  // std::cout << "Try to deregistered " << channel->GetFd() << " from epoll:" << poller_fd_ << std::endl;
  Errif(epoll_ctl(poller_fd_, EPOLL_CTL_DEL, channel->GetFd(), nullptr) == -1, "epoll delete error");
}
#endif

#ifdef OS_MACOS
const int MAXEVENTS = 1024;
Poller::Poller() {
  poller_fd_ = kqueue();
  Errif(poller_fd_ == -1, "kqueue create error");
  events_ = new kevent[MAXEVENTS];
  Errif(events_ == nullptr, "kqueue events new error");
  memset(events_, 0, sizeof(*events_) * MAXEVENTS);
}

Poller::~Poller() {
  ::close(poller_fd_);
  delete[] events_;
}
std::vector<Channel *> Poller::Poll(int timeout) {
  // std::cout << "begin:EventLoop::Loop() epoll_fd_ = " << epoll_fd_ << std::endl;
  std::vector<Channel *> active_channels;
  // std::cout << "begin:EventLoop::Loop() events_ = " << events_ << std::endl;
  struct timespec ts;
  ts.tv_sec = timeout / 1000;
  ts.tv_nsec = (timeout % 1000) * 1000000;
  if (timeout == -1) {
    int nfds = kevent(poller_fd_, nullptr, 0, events_, MAXEVENTS, nullptr);
  } else {
    int nfds = kevent(poller_fd_, nullptr, 0, events_, MAXEVENTS, &ts);
  }
  for (int i = 0; i < nfds; ++i) {
    // std::cout << "accept:EventLoop::Loop() epoll_fd_"<< epoll_fd_ << " , events_[" << events_[i].events <<
    // "].data.ptr = " << events_[i].data.ptr << std::endl;
    Channel *ch = static_cast<Channel *>(events_[i].udata);
    ch->SetReadyEvents(0);
    if (events_[i].filter == EVFILT_READ) {
      ch->SetReadyEvents(Channel::READ_EVENT);
    }
    if (events_[i].filter == EVFILT_WRITE) {
      ch->SetReadyEvents(Channel::WRITE_EVENT);
    }
    active_channels.push_back(ch);
  }
  return active_channels;
}

void Poller::UpdateChannel(Channel *channel) {
  struct kevent ev[2];
  memset(ev, 0, sizeof(ev) * 2);
  int n = 0;  // kevent计数器
  int fd = channel->GetFd();
  int op = EV_ADD;  // 操作类型
  if (channel->GetListenEvents() & Channel::ET_EVENT) {
    op |= EV_CLEAR;  // 边缘触发
  }
  if (channel->GetListenEvents() & Channel::READ_EVENT) {
    EV_SET(&ev[n++], fd, EVFILT_READ, op, 0, 0, channel)  // channel作为udata方便事件分发。
  }
  if (channel->GetListenEvents() & Channel::WRITE_EVENT) {
    EV_SET(&ev[n++], fd, EVFILT_WRITE, op, 0, 0, channel)  // channel作为udata方便事件分发。
  }
  Errif(kevent(poller_fd_, ev, n, nullptr, 0, nullptr) == -1, "kqueue update error");

  // std::cout << "Registered "<< channel->GetFd() << " in kevent:" << poller_fd_ << std::endl;
}

void Poller::DeleteChannel(Channel *channel) {
  struct kevent ev[2];
  memset(ev, 0, sizeof(ev) * 2);
  int n = 0;
  if (channel->GetListenEvents() & Channel::READ_EVENT) {
    EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, channel)  // channel作为udata方便事件分发。
  }
  if (channel->GetListenEvents() & Channel::WRITE_EVENT) {
    EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, channel)  // channel作为udata方便事件分发。
  }
  Errif(kevent(poller_fd_, ev, n, nullptr, 0, nullptr) == -1, "kqueue delete error");
}
#endif
