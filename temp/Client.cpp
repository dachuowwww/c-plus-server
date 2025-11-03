#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "Buffer.h"
#include "error.h"
#include "fcntl.h"
const int CLNT_BUFFER = 1024;
using std::cout;
using std::endl;
using std::string;

struct sockaddr_in serv_addr;

int main() {
  Buffer outputBuffer;
  Buffer inputBuffer;
  bzero(&serv_addr, sizeof(serv_addr));

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  errif(sockfd == -1, "socket create error");

  serv_addr.sin_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
  serv_addr.sin_port = htons(8888);
  //源ip和port会自动分配
  errif(connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");
  cout << "connected to server ! " << endl;
  while (true) {
    cout << "Please input message to send to server :" << endl;
    outputBuffer.getLine();
    ssize_t write_bytes = write(sockfd, outputBuffer.readAll(),
                                outputBuffer.getSize());  // 发送缓冲区中的数据到服务器socket，返回已发送数据大小
    if (write_bytes == -1) {                              // write返回-1，表示发生错误
      cout << "socket already disconnected, can't write any more!" << endl;
      break;
    }
    outputBuffer.clear();  // 清空缓冲区
    char buf[CLNT_BUFFER]{};
    ssize_t read_bytes = read(sockfd, buf, sizeof(buf));  //从服务器socket读到缓冲区，返回已读数据大小
    if (read_bytes > 0) {
      inputBuffer.append(buf, read_bytes);
    } else if (read_bytes == 0) {  // read返回0，表示EOF，通常是服务器断开链接，等会儿进行测试
      close(sockfd);
      cout << "server socket disconnected!" << endl;
      return 0;
    } else if (read_bytes == -1 && errno == EINTR) {  //客户端正常中断、继续读取
      cout << "continue reading" << endl;
      continue;
    } else if (read_bytes == -1 &&
               ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {  //非阻塞IO，这个条件表示数据全部读取完毕
      cout << "message from server: " << inputBuffer.readAll() << endl;
      //该fd上数据读取完毕
      break;
    } else if (read_bytes == -1) {  // read返回-1，表示发生错误，按照上文方法进行错误处理
      close(sockfd);
      errif(true, "socket read error");
    }
    inputBuffer.clear();  //清空缓冲区
  }
  close(sockfd);
  return 0;
}
