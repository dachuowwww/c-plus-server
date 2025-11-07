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
  sock->Connect();
  // int flags = fcntl(sock->GetFd(), F_GETFL, 0);
  // fcntl(sock->GetFd(), F_SETFL, flags | O_NONBLOCK);
  Connection cln_conn(nullptr, sock);
  while (true) {
    cout << "Please input message to send to server :" << endl;
    cln_conn.ReadKeyBoard();
    cln_conn.SetOutput(cln_conn.ReadInputBuffer());
    cln_conn.Write();
    if ((cln_conn.GetState() == Connection::State::Closed)) {
      cln_conn.Close();
      break;
    }
    cln_conn.Read();
    cout << "message from server : " << cln_conn.ReadInputBuffer() << endl;
  }
  return 0;
}
