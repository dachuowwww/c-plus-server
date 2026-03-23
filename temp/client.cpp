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
    cln_conn.KeyBoardToOutput();
    cln_conn.Write();
    if ((cln_conn.GetState() == Connection::State::Closed)) {
      cout << "client write error, exit" << endl;
      break;
    }
    cln_conn.Read();
    if ((cln_conn.GetState() == Connection::State::Closed)) {
      cout << "client read error, exit" << endl;
      break;
    }
    cout << "message from server : " << cln_conn.RetriveInputBuffer() << endl;
  }
  return 0;
}
