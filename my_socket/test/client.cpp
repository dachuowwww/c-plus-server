#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "Buffer.h"
#include "Error.h"
#include "fcntl.h"
const int CLNT_BUFFER = 1024;
using std::cout;
using std::endl;
using std::string;

struct sockaddr_in serv_addr;

int main() {
  Buffer output_buffer;
  Buffer input_buffer;
  memset(&serv_addr, 0, sizeof(serv_addr));

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  Errif(sockfd == -1, "socket create error");

  serv_addr.sin_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
  serv_addr.sin_port = htons(8888);
  // 源ip和port会自动分配
  Errif(connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");
  cout << "connected to server ! " << endl;
  while (true) {
    cout << "Please input message to send to server :" << endl;
    output_buffer.GetKeyboardLine();
    ssize_t write_bytes = write(sockfd, output_buffer.ReadAll(),
                                output_buffer.GetSize());  // 发送缓冲区中的数据到服务器socket，返回已发送数据大小
    if (write_bytes == -1) {                               // write返回-1，表示发生错误
      cout << "socket already disconnected, can't write any more!" << endl;
      break;
    }
    output_buffer.Clear();  // 清空缓冲区
    char buf[CLNT_BUFFER]{};
    ssize_t read_bytes = read(sockfd, buf, sizeof(buf));  // 从服务器socket读到缓冲区，返回已读数据大小
    if (read_bytes > 0) {
      input_buffer.GetData(buf);
      cout << "send message successfully, message from server: " << input_buffer.ReadAll() << endl;
      input_buffer.Clear();        // 清空缓冲区
    } else if (read_bytes == 0) {  // read返回0，表示EOF，通常是服务器断开链接，等会儿进行测试
      close(sockfd);
      cout << "server socket disconnected!" << endl;
      return 0;
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
  }
  close(sockfd);
  return 0;
}
