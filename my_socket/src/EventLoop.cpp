#include "EventLoop.h"
#include <iostream>
#include <vector>
#include "Channel.h"
#include "Epoll.h"
#include "ThreadPool.h"

EventLoop::EventLoop(/* args */) { ep_.reset(new Epoll()); }

EventLoop::~EventLoop() {}

void EventLoop::Update(Channel *channel) {
  if (!(channel->IfInEpoll())) {
    ep_->AddChannel(channel);

  } else {
    ep_->UpdateChannel(channel);
  }
}
void EventLoop::Loop() {
  while (true) {
    std::vector<Channel *> chs;
    chs = ep_->Poll();
    for (auto it : chs) {
      it->HandleEvent();
    }
  }
}

void EventLoop::Delete(Channel *channel) { ep_->DeleteChannel(channel); }
