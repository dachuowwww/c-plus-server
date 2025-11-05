#include <fcntl.h>
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
// 设置socket读超时
void SetSocketTimeout(int sockfd, int seconds) {
  struct timeval timeout {};
  timeout.tv_sec = seconds;
  timeout.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    perror("setsockopt failed");
  }
}

void OneClient(int msgs, int wait) {
  std::shared_ptr<InetAddress> addr = make_shared<InetAddress>("127.0.0.1", 8888);
  TCPSocket *sock = new TCPSocket(addr);
  sock->Connect();
  // int flags = fcntl(sock->GetFd(), F_GETFL, 0);
  // fcntl(sock->GetFd(), F_SETFL, flags | O_NONBLOCK);

  SetSocketTimeout(sock->GetFd(), 10);  // 10秒超时时间

  int sockfd = sock->GetFd();
  cout << "thread " << std::this_thread::get_id() << " socket fd = " << sockfd << endl;

  Buffer output_buffer{};
  Buffer input_buffer{};

  sleep(wait);
  int count = 0;
  char msg[] = "I'm client!";
  output_buffer.GetData(msg);
  while (count < msgs) {
    ssize_t write_bytes = write(sockfd, output_buffer.ReadAll(),
                                output_buffer.GetSize());  // 发送缓冲区中的数据到服务器socket，返回已发送数据大小
    if (write_bytes == -1) {                               // write返回-1，表示发生错误
      cout << "socket already disconnected, can't write any more!" << endl;
      break;
    }
    char buf[1024]{};
    ssize_t read_bytes = read(sockfd, buf, sizeof(buf));  // 从服务器socket读到缓冲区，返回已读数据大小
    if (read_bytes > 0) {
      input_buffer.GetData(buf);
      cout << "no." << count++ << " message from server: " << input_buffer.ReadAll() << endl;
      input_buffer.Clear();        // 清空缓冲区
    } else if (read_bytes == 0) {  // read返回0，表示EOF，通常是服务器断开链接，等会儿进行测试
      close(sockfd);
      cout << "server socket disconnected!" << endl;
      return;
    } else if (read_bytes == -1 && errno == EINTR) {  // 服务端正常中断、继续读取
      cout << "continue reading" << endl;
      continue;
      // } else if (read_bytes == -1 &&
      //            ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {  // 非阻塞IO，这个条件表示数据全部读取完毕
      //   cout << "message from server: " << input_buffer.ReadAll() << endl;
      //   // 该fd上数据读取完毕
      //   break;
    } else if (read_bytes == -1) {  // read返回-1，表示发生错误，按照上文方法进行错误处理
      close(sockfd);
      Errif(true, "socket read error");
    }
    // while (count < msgs) {
    //   size_t write_bytes = write(sockfd, send_buffer.ReadAll(), send_buffer.GetSize());
    //   cout << "client had sent: " << msg << endl;
    //   if (write_bytes == 0) {
    //     printf("socket already disconnected, can't write any more!\n");
    //     break;
    //   }
    //   // int already_read = 0;
    //   char buf[1024] = {};  // 这个buf大小无所谓
    //   while (true) {
    //     ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
    //     if (read_bytes > 0) {
    //       read_buffer.GetData(buf);
    //       printf("count: %d, message from server: %s\n", count++, read_buffer.ReadAll());
    //       break;
    //       // already_read += read_bytes;
    //     } else if (read_bytes == 0) {  // EOF
    //       printf("server disconnected!\n");
    //       exit(EXIT_SUCCESS);
    //      } //else if (read_bytes == -1 && errno == EINTR) {
    //     //   // 中断，继续读取
    //     //   continue;
    //     // } else if (read_bytes == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
    //     //   // 读超时或者数据读完
    //     //   break;
    //     // } else if (read_bytes == -1) {
    //     //   Errif(true, "socket read error");
    //     // }
    //     // if (already_read >= send_buffer.GetSize()) {

    //     //   already_read = 0;
    //     //   break;
    //     // }
    //   }
    //   read_buffer.Clear();
    // }
  }
  delete sock;
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

  ThreadPool *poll = new ThreadPool(threads);
  for (int i = 0; i < threads; ++i) {
    poll->Add(OneClient, msgs, wait);
  }
  //usleep(100000);
  delete poll;
  return 0;
}
