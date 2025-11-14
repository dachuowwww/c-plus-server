#include "Server.h"
#include <iostream>
#include <memory>
#include "Connection.h"
#include "EventLoop.h"
//#include <csignal>

int main() {
  //signal(SIGPIPE, SIG_IGN);
  auto loop = std::make_unique<EventLoop>();
  Server server(loop.get());
  server.OnConnect([](Connection *conn) {  // 注册回调函数,需要修改内部元素所以不能设为const
    // conn->Read();
    if ((conn->GetState() == Connection::State::Closed)) {
      conn->RemoveConnection();
      return;
    }
    std::cout << "new message from client " << conn->GetFd() << " : " << conn->ReadInputBuffer() << std::endl;
    conn->Send(conn->ReadInputBuffer());
    if ((conn->GetState() == Connection::State::Closed)) {
      conn->RemoveConnection();
    }
  });
  loop->Loop();
  return 0;
}
