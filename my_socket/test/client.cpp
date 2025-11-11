#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "Buffer.h"
#include "Connection.h"
#include "Error.h"
#include "InetAddress.h"
#include "Socket.h"

using std::cout;
using std::endl;
using std::string;

struct sockaddr_in serv_addr;

int main() {
  std::shared_ptr<InetAddress> addr = std::make_shared<InetAddress>("127.0.0.1", 8888);
  auto sock = std::make_shared<Socket>(addr);
  // sock->SetNonBlocking();
  sock->Connect();
  Connection cln_conn(nullptr, sock);
  while (true) {
    cln_conn.KeyBoardInput();
    cln_conn.SetOutput(cln_conn.ReadInputBuffer());
    cln_conn.Write();
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
