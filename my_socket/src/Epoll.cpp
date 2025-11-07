#include "Epoll.h"
#include <cstring>
#include <iostream>
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
  std::vector<Channel *> active_channels;
  int nfds = epoll_wait(epoll_fd_, events_, MAXEVENTS, timeout);
  for (int i = 0; i < nfds; ++i) {
    Channel *ch = static_cast<Channel *>(events_[i].data.ptr);
    ch->SetRevents(events_[i].events);
    active_channels.push_back(ch);
  }
  return active_channels;
}
void Epoll::AddChannel(Channel *channel) {
  memset(&ev_, 0, sizeof(struct epoll_event));
  ev_.events = channel->GetEvents();
  ev_.data.ptr = channel;
  Errif(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, channel->GetFd(), &ev_) == -1, "epoll add error");
  channel->SetInEpoll();
}
void Epoll::UpdateChannel(Channel *channel) {
  memset(&ev_, 0, sizeof(struct epoll_event));
  ev_.events = channel->GetEvents();
  ev_.data.ptr = channel;
  Errif(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, channel->GetFd(), &ev_) == -1, "epoll modify error");
}
void Epoll::DeleteChannel(Channel *channel) {
  memset(&ev_, 0, sizeof(struct epoll_event));
  ev_.events = channel->GetEvents();
  ev_.data.ptr = channel;
  Errif(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, channel->GetFd(), &ev_) == -1, "epoll delete error");
}
