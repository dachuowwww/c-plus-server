#include "EventLoop.h"
#include <iostream>
#include <vector>
#include "Channel.h"
#include "Epoll.h"
#include "ThreadPool.h"

EventLoop::EventLoop(/* args */) { ep_ = std::make_unique<Epoll>(); }
EventLoop::~EventLoop() = default;
void EventLoop::Update(Channel *channel) { ep_->UpdateChannel(channel); }
void EventLoop::Loop() {
  while (true) {
    std::vector<Channel *> chs;
    chs = ep_->Poll();
    for (auto *it : chs) {  // more clare
      it->HandleEvent();
    }
  }
}

void EventLoop::Delete(Channel *channel) { ep_->DeleteChannel(channel); }
