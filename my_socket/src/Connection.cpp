#include "Connection.h"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "Buffer.h"
#include "Channel.h"
#include "Error.h"
#include "EventLoop.h"
#include "Socket.h"
using std::cout;
using std::endl;
using std::function;
using std::shared_ptr;

const int SERV_BUFFER = 1024;
Connection::Connection(shared_ptr<EventLoop> loop, shared_ptr<Socket> socket)
    : loop_(std::move(loop)), conn_socket_(std::move(socket)) {
  if (loop_ != nullptr) {
    conn_channel_ = std::make_unique<Channel>(loop_, conn_socket_->GetFd());
  }
  input_buffer_ = std::make_unique<Buffer>();
  output_buffer_ = std::make_unique<Buffer>();
  SetState(State::Connected);
}
Connection::~Connection() = default;
void Connection::SetRemoveConnection(function<void(shared_ptr<Socket> &)> cb) { remove_ = std::move(cb); }
bool Connection::IsInEpoll() const { return conn_channel_->IfInEpoll(); }
int Connection::GetFd() const { return conn_channel_->GetFd(); }
void Connection::SetET() { conn_channel_->UseET(); }
void Connection::EnableReading() { conn_channel_->EnableReading(); }
// void Connection::Echo() {
//   if (!IsConnected()) {
//     cout << "client fd " << conn_socket_->GetFd() << " disconnected" << endl;
//     return;
//   }
//   char buf[SERV_BUFFER];
//   while (true) {
//     memset(&buf, 0, sizeof(buf));
//     ssize_t bytes_read = read(conn_socket_->GetFd(), buf, sizeof(buf));
//     if (bytes_read > 0) {
//       input_buffer_->Input(buf);
//       // cout<<bytes_read<<" bytes message from client fd "<<conn_socket->GetFd()<<" : "<<buf<<endl;
//       // write(conn_socket_->GetFd(), buf, bytes_read);
//     } else if (bytes_read == -1 && errno == EINTR) {  // 客户端正常中断、继续读取
//       cout << "continue reading" << endl;
//       continue;
//     } else if (bytes_read == -1 &&
//                ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {  // 非阻塞IO，这个条件表示数据全部读取完毕
//       cout << "finish reading once, errno: " << errno << endl;
//       if (input_buffer_->GetSize() > 0) {
//         cout << "message from client fd " << conn_socket_->GetFd() << " : " << input_buffer_->GetData() << endl;
//         Errif(write(conn_socket_->GetFd(), input_buffer_->Output(), input_buffer_->GetSize()) == -1, "write error");
//         input_buffer_->Clear();
//       }
//       break;
//     } else if (bytes_read == 0) {  // EOF，客户端断开连接
//       cout << "EOF, client fd " << conn_socket_->GetFd() << " disconnected" << endl;
//       RemoveConnection();
//       break;
//     } else {
//       perror("read error");
//       RemoveConnection();
//       break;
//     }
//   }
// }
void Connection::Bussiness() {
  Read();
  handle_read_func_(this);
}

void Connection::SetHandleReadFunc(function<void(Connection *)> cb) {
  handle_read_func_ = std::move(cb);
  // conn_channel_->SetReadCallback([this]() { handle_read_func_(this); });
  conn_channel_->SetReadCallback(std::bind(&Connection::Bussiness, this));  // 调用可省略，取地址不行。
}

void Connection::Send(const char *data) {
  output_buffer_->SetData(data);
  Write();
}
void Connection::Read() {
  Errif(state_ != State::Connected, "connection not connected");  // 静态断言，如果断言失败，程序终止
  input_buffer_->Clear();                                         // 不能去，因为非租塞使用append处理数据
  if (conn_socket_->IsNonBlocking()) {
    Connection::ReadNonBlocking();  // 程序进行中进行连接判断，如果连接正常，进行阻塞IO读取
  } else {
    Connection::ReadBlocking();
  }
}

void Connection::Write() {
  Errif(state_ != State::Connected, "connection not connected");
  if (conn_socket_->IsNonBlocking()) {
    Connection::WriteNonBlocking();
  } else {
    Connection::WriteBlocking();
  }
  output_buffer_->Clear();
}

void Connection::ReadNonBlocking() {
  // std::cout << "non-blocking read" << std::endl;
  char buf[SERV_BUFFER];
  while (true) {
    memset(buf, 0, sizeof(buf));
    ssize_t bytes_read = read(conn_socket_->GetFd(), buf, sizeof(buf));
    if (bytes_read > 0) {
      input_buffer_->Append(buf, bytes_read);
      continue;
    }
    if (bytes_read == -1 && errno == EINTR) {  // 对方正常中断、继续读取
      cout << "continue reading" << endl;
      continue;
    }
    if (bytes_read == -1 &&
        ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {  // 非阻塞IO，这个条件表示数据全部读取完毕；读到 EAGAIN
                                                          // 时立即返回，再由事件机制在下一次有新数据到来时重新触发。
      cout << "finish reading once, errno: " << errno << endl;
      break;
    }
    if (bytes_read == 0) {  // EOF，对方断开连接
      cout << "EOF, fd " << conn_socket_->GetFd() << " disconnected" << endl;
      SetState(State::Closed);
      break;
    }
    cout << "other read error" << endl;
    SetState(State::Closed);
    break;
  }
}

void Connection::ReadBlocking() {
  // std::cout << "blocking read" << std::endl;
  char buf[SERV_BUFFER];
  while (true) {
    memset(buf, 0, sizeof(buf));
    ssize_t bytes_read = read(conn_socket_->GetFd(), buf, sizeof(buf));
    if (bytes_read > 0) {
      input_buffer_->SetData(buf);
      break;
    }
    if (bytes_read == -1 && errno == EINTR) {  // 对方正常中断、继续读取
      cout << "continue reading" << endl;
      continue;
    }
    if (bytes_read == 0) {  // EOF，对方断开连接
      cout << "EOF, fd " << conn_socket_->GetFd() << " disconnected" << endl;
      SetState(State::Closed);
      break;
    }
    cout << "other read error" << endl;
    SetState(State::Closed);
    break;
  }
}

void Connection::WriteNonBlocking() {
  // std::cout << "non-blocking write" << std::endl;
  std::string buf = output_buffer_->GetData();
  size_t bytes_written = 0;
  size_t bytes_to_write = output_buffer_->GetSize();
  int fd = conn_socket_->GetFd();
  while (true) {
    ssize_t n = write(fd, buf.c_str() + bytes_written, bytes_to_write - bytes_written);
    if (n > 0) {
      bytes_written += n;
      if (bytes_written == bytes_to_write) {
        break;
      }
      continue;
    }
    if (n == -1 && errno == EINTR) {  // 对方正常中断、继续写入
      cout << "continue writing" << endl;
      continue;
    }
    if (n == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {  // 非阻塞IO，这个条件表示缓冲区已满，需要等待
      cout << "buffer full, wait for writing" << endl;
      break;
    }
    cout << "other write error" << endl;
    SetState(State::Closed);
    break;
  }
}
void Connection::WriteBlocking() {
  // std::cout << "blocking write" << std::endl;
  while (true) {
    ssize_t n = write(conn_socket_->GetFd(), output_buffer_->GetData(), output_buffer_->GetSize());
    if (n > 0) {
      //std::cout << "write " << n << " bytes , content: " << output_buffer_->GetData() << std::endl;
      break;
    }
    if (n == -1 && errno == EINTR) {  // 对方正常中断、继续写入
      cout << "continue writing" << endl;
      continue;
    }
    cout << "other write error" << endl;
    SetState(State::Closed);
    break;
  }
}

void Connection::KeyBoardInput() { input_buffer_->SetKeyBoardInput(); }

const char *Connection::ReadInputBuffer() const { return input_buffer_->GetData(); }

void Connection::SetOutput(const char *data) { output_buffer_->SetData(data); }

void Connection::RemoveConnection() {
  conn_channel_->RemoveInEpoll();
  remove_(conn_socket_);
}  // 关闭进行中的读写操作，并移除连接

void Connection::SetState(State state) { state_ = state; }

Connection::State Connection::GetState() const { return state_; }
