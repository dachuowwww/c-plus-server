#pragma once

#include <sys/epoll.h>
#include <vector>
const int MAXEVENTS = 1024;
class Channel;

class Epoll {
 private:
  int epoll_fd_;
  struct epoll_event *events_;
  struct epoll_event ev_;

 public:
  Epoll(/* args */);
  ~Epoll();
  Epoll(const Epoll &) = delete;
  Epoll &operator=(const Epoll &) = delete;
  Epoll(Epoll &&) = delete;
  Epoll &operator=(Epoll &&) = delete;
  std::vector<Channel *> Poll(int timeout = -1);
  void AddChannel(Channel *);  // 后期需要修改 还传递给void 所以不能改成const
  void UpdateChannel(Channel *);
  void DeleteChannel(Channel *);
};
