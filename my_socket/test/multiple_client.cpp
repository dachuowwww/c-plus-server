#include <unistd.h>
#include <cstring>
#include <functional>
#include <iostream>
#include "Buffer.h"
#include "Error.h"
#include "InetAddress.h"
#include "Socket.h"
#include "ThreadPool.h"

using std::cout;
using std::endl;
using std::make_shared;
using std::stoi;
void SetSocketTimeout(int sockfd, int seconds) {
  struct timeval timeout = {0, 0};
  timeout.tv_sec = seconds;

  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

void OneClient(int msgs, int wait) {
  std::shared_ptr<InetAddress> addr = make_shared<InetAddress>("127.0.0.1", 8888);
  TCPSocket *sock = new TCPSocket(addr);
  sock->Connect();
  SetSocketTimeout(sock->GetFd(), 20);  // 20秒超时时间

  int sockfd = sock->GetFd();

  Buffer *send_buffer = new Buffer();
  Buffer *read_buffer = new Buffer();

  sleep(wait);
  int count = 0;
  while (count < msgs) {
    char msg[] = "I'm client!";
    send_buffer->Append(msg, strlen(msg));
    size_t write_bytes = write(sockfd, send_buffer->ReadAll(), send_buffer->GetSize());
    cout << "client had sent: " << msg << endl;
    if (write_bytes == 0) {
      printf("socket already disconnected, can't write any more!\n");
      break;
    }
    int already_read = 0;
    char buf[1024];  // 这个buf大小无所谓
    while (true) {
      bzero(&buf, sizeof(buf));
      ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
      if (read_bytes > 0) {
        read_buffer->Append(buf, read_bytes);
        already_read += read_bytes;
      } else if (read_bytes == 0) {  // EOF
        printf("server disconnected!\n");
        exit(EXIT_SUCCESS);
      }
      if (already_read >= send_buffer->GetSize()) {
        printf("count: %d, message from server: %s\n", count++, read_buffer->ReadAll());
        break;
      }
    }
    send_buffer->Clear();
    read_buffer->Clear();
  }
  delete sock;
}

int main(int argc, char *argv[]) {
  int threads = 100;
  int msgs = 100;
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

  ThreadPool *poll = new ThreadPool(threads);
  for (int i = 0; i < threads; ++i) {
    poll->Add(OneClient, msgs, wait);
  }
  delete poll;
  return 0;
}
