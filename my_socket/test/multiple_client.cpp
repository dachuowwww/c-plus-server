#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <functional>
#include <iostream>

#include "Connection.h"
#include "Error.h"
#include "Socket.h"
#include "ThreadPool.h"

using std::cout;
using std::endl;
using std::make_shared;
using std::stoi;
// 设置socket读超时
// void SetSocketTimeout(int sockfd, int seconds) {
//   struct timeval timeout {};
//   timeout.tv_sec = seconds;
//   timeout.tv_usec = 0;
//   if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
//     perror("setsockopt failed");
//   }
// }

void OneClient(int msgs, int wait) {
  auto sock = std::make_unique<Socket>();
  sock->Connect("127.0.0.1", 8888);
  // int flags = fcntl(sock->GetFd(), F_GETFL, 0);
  // fcntl(sock->GetFd(), F_SETFL, flags | O_NONBLOCK);

  // SetSocketTimeout(sock->GetFd(), 10);  // 10秒超时时间

  // int sockfd = sock->GetFd();
  // cout << "thread " << std::this_thread::get_id() << " socket fd = " << sockfd << endl;
  Connection cln_conn(nullptr, sock->GetFd());

  sleep(wait);
  int count = 0;
  while (count < msgs) {
    cln_conn.Send("I'm client!");
    if ((cln_conn.GetState() == Connection::State::Closed)) {
      break;
    }
    cln_conn.Read();
    if ((cln_conn.GetState() == Connection::State::Closed)) {
      break;
    }
    cout << "Client fd: " << cln_conn.GetFd() << " accept no." << count++
         << " message from server : " << cln_conn.ReadInputBuffer() << endl;
  }
}

int main(int argc, char *argv[]) {
  int threads = 100;
  int msgs = 10;
  int wait = 0;
  int o = 0;
  const char *optstring = "t:m:w:";
  while ((o = getopt(argc, argv, optstring)) != -1) {
    switch (o) {
      case 't':
        threads = stoi(optarg);
        break;
      case 'm':
        msgs = stoi(optarg);
        break;
      case 'w':
        wait = stoi(optarg);
        break;
      case '?':
        printf("error optopt: %c\n", optopt);
        printf("error opterr: %d\n", opterr);
        break;
      default:
        break;
    }
  }

  auto poll = std::make_unique<ThreadPool>(threads);
  for (int i = 0; i < threads; ++i) {
    poll->Add(OneClient, msgs, wait);
  }
  // usleep(100000);
  return 0;
}
