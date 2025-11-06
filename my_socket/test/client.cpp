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
  auto sock = std::make_shared<TCPSocket>(addr);
  sock->Connect();
  // int flags = fcntl(sock->GetFd(), F_GETFL, 0);
  // fcntl(sock->GetFd(), F_SETFL, flags | O_NONBLOCK);
  Connection cln_conn(nullptr, sock);
  while (true) {
    cout << "Please input message to send to server :" << endl;
    cln_conn.ReadKeyBoard();
    cln_conn.Write();
    if (!(cln_conn.IsConnected())) {
      cln_conn.Close();
      break;
    }
    cln_conn.Read();
    cout << "message from server : " << cln_conn.ReadBuffer() << endl;
  }
  return 0;
}
