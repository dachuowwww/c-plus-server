#include "EventLoop.h"
#include <iostream>
#include <vector>
#include "Channel.h"
#include "Poller.h"
#include "ThreadPool.h"

EventLoop::EventLoop(/* args */) { poller_ = std::make_unique<Poller>(); }
EventLoop::~EventLoop() = default;
void EventLoop::Update(Channel *channel) { poller_->UpdateChannel(channel); }

void EventLoop::Loop() {
  while (true) {
    std::vector<Channel *> chs;
    chs = poller_->Poll();
    for (auto *it : chs) {  // more clare
      it->HandleEvent();
    }
  }
}

void EventLoop::Delete(Channel *channel) { poller_->DeleteChannel(channel); }
