#include "Server.h"
#include "EventLoop.h"
int main() {
  EventLoop loop;
  Server server(&loop);
  // loop.OpenThreadPool();
  loop.Loop();
  return 0;
}
