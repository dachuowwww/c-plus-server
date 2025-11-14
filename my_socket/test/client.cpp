#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "Connection.h"
#include "Socket.h"

using std::cout;
using std::endl;
using std::string;

struct sockaddr_in serv_addr;

int main() {
  auto sock = std::make_unique<Socket>();
  // sock->SetNonBlocking();
  sock->Connect("127.0.0.1", 8888);
  Connection cln_conn(nullptr, sock->GetFd());
  while (true) {
    cln_conn.KeyBoardInput();
    cln_conn.Send(cln_conn.ReadInputBuffer());
    if ((cln_conn.GetState() == Connection::State::Closed)) {
      break;
    }
    cln_conn.Read();
    if ((cln_conn.GetState() == Connection::State::Closed)) {
      break;
    }
    cout << "message from server : " << cln_conn.ReadInputBuffer() << endl;
  }
  return 0;
}
