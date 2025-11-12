#pragma once
#ifdef LINUX
#include <sys/epoll.h>
#endif
#ifdef MACOS
#include <sys/event.h>
#endif
#include <vector>
#include "Macro.h"

class Channel;

class Poller {
 private:
  int poller_fd_ = 0;
#ifdef OS_LINUX
  struct epoll_event *events_ = nullptr;
#endif
#ifdef OS_MACOS
  struct kevent *events_ = nullptr;
#endif

 public:
  Poller();
  ~Poller();
  std::vector<Channel *> Poll(int timeout = -1);
  void UpdateChannel(Channel *channel);  // 后期需要修改 还传递给void 所以不能改成const
  void DeleteChannel(Channel *channel);

  DISALLOW_COPY_AND_ASSIGN(Poller);
};
