#pragma once
#include <memory>
#include "Macro.h"
class Channel;
class Poller;
class EventLoop {
 public:
  EventLoop();
  ~EventLoop();
  void Update(Channel *channel);
  void Loop();
  void Delete(Channel *channel);

 private:
  std::unique_ptr<Poller> poller_;

  DISALLOW_COPY_AND_ASSIGN(EventLoop);
};
