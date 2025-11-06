#include "Server.h"
#include <iostream>
#include "Connection.h"
#include "EventLoop.h"
int main() {
  EventLoop loop;
  Server server(&loop);
  server.OnConnect([](Connection *conn) {  // 注册回调函数,需要修改内部元素所以不能设为const
    conn->Read();
    std::cout << "new message from client " << conn->GetFd() << " : " << conn->ReadBuffer() << std::endl;
    if (!(conn->IsConnected())) {
      conn->RemoveConnection();
      return;
    }
    conn->SetOutput(conn->ReadBuffer());
    conn->Write();
  });
  // loop.OpenThreadPool();
  loop.Loop();
  return 0;
}
