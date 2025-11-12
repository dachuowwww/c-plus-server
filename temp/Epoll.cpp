#include "Epoll.h"
#include <cstring>
#include "Channel.h"
#include "Error.h"

const int MAXEVENTS = 1024;
Epoll::Epoll() {
  epoll_fd_ = epoll_create1(0);
  Errif(epoll_fd_ == -1, "epoll create error");
  events_ = new epoll_event[MAXEVENTS];
  Errif(events_ == nullptr, "epoll events new error");
  memset(events_, 0, sizeof(*events_) * MAXEVENTS);
}
std::vector<Channel *> Epoll::Poll(int timeout) {
  // std::cout << "begin:EventLoop::Loop() epoll_fd_ = " << epoll_fd_ << std::endl;
  std::vector<Channel *> active_channels;
  // std::cout << "begin:EventLoop::Loop() events_ = " << events_ << std::endl;
  int nfds = epoll_wait(epoll_fd_, events_, MAXEVENTS, timeout);
  for (int i = 0; i < nfds; ++i) {
    // std::cout << "accept:EventLoop::Loop() epoll_fd_"<< epoll_fd_ << " , events_[" << events_[i].events <<
    // "].data.ptr = " << events_[i].data.ptr << std::endl;
    Channel *ch = static_cast<Channel *>(events_[i].data.ptr);
    ch->SetRevents(events_[i].events);
    active_channels.push_back(ch);
  }
  return active_channels;
}

void Epoll::UpdateChannel(Channel *channel) {
  struct epoll_event ev = {};
  ev.events = channel->GetEvents();
  ev.data.ptr = channel;
  if (!channel->IfInEpoll()) {
    Errif(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, channel->GetFd(), &ev) == -1, "epoll add error");
    channel->SetInEpoll();
  } else {
    Errif(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, channel->GetFd(), &ev) == -1, "epoll modify error");
  }
  // std::cout << "Registered "<< channel->GetFd() << " in epoll:" << epoll_fd_ << std::endl;
}
void Epoll::DeleteChannel(Channel *channel) {
  struct epoll_event ev = {};
  ev.events = channel->GetEvents();
  ev.data.ptr = channel;
  Errif(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, channel->GetFd(), &ev) == -1, "epoll delete error");
}
