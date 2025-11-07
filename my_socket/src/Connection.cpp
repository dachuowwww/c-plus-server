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
    : loop_(std::move(loop)),
      conn_socket_(std::move(socket)),
      input_buffer_(std::make_unique<Buffer>()),
      output_buffer_(std::make_unique<Buffer>()) {
  if (loop_ != nullptr) {
    conn_channel_ = std::make_unique<Channel>(loop_, conn_socket_->GetFd());

    function<void()> cb1 = std::bind(&Connection::HandleRead, this);
    conn_channel_->SetReadCallback(cb1);
    function<void()> cb2 = std::bind(&Connection::RemoveConnection, this);
    conn_channel_->SetCloseCallback(cb2);
    SetState(State::Connected);
  } else {
    conn_channel_ = nullptr;
    SetState(State::Invaild);
  }
}
Connection::~Connection() = default;
void Connection::SetRemoveConnection(const function<void(shared_ptr<Socket> &)> &cb) { remove_ = cb; }
bool Connection::IsInEpoll() const { return conn_channel_->IfInEpoll(); }
int Connection::GetFd() const { return conn_channel_->GetFd(); }
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

void Connection::HandleRead() {
  handle_read_func_(this);
  // } else {
  //   Connection::Echo();
  // }
}

void Connection::SetHandleReadFunc(const function<void(Connection *)> &cb) { handle_read_func_ = cb; }

void Connection::Read() {
  input_buffer_->Clear();
  if (conn_socket_->IsBlocking()) {
    Connection::ReadBlocking();
  } else {
    Connection::ReadNonBlocking();
  }
}

void Connection::Write() {
  if (conn_socket_->IsBlocking()) {
    Connection::WriteBlocking();
  } else {
    Connection::WriteNonBlocking();
  }
  output_buffer_->Clear();
}

void Connection::ReadNonBlocking() {
  char buf[SERV_BUFFER];
  while (true) {
    memset(buf, 0, sizeof(buf));
    ssize_t bytes_read = read(conn_socket_->GetFd(), buf, sizeof(buf));
    if (bytes_read > 0) {
      input_buffer_->Input(buf);
      continue;
    }
    if (bytes_read == -1 && errno == EINTR) {  // 对方正常中断、继续读取
      cout << "continue reading" << endl;
      continue;
    }
    if (bytes_read == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {  // 非阻塞IO，这个条件表示数据全部读取完毕
      cout << "finish reading once, errno: " << errno << endl;
      break;
    }
    if (bytes_read == 0) {  // EOF，对方断开连接
      cout << "EOF, fd " << conn_socket_->GetFd() << " disconnected" << endl;
      SetState(State::Closed);
      break;
    }
    perror("read error");
    break;
  }
}

void Connection::ReadBlocking() {
  char buf[SERV_BUFFER];
  while (true) {
    memset(buf, 0, sizeof(buf));
    ssize_t bytes_read = read(conn_socket_->GetFd(), buf, sizeof(buf));
    if (bytes_read > 0) {
      input_buffer_->Input(buf);
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
    perror("read error");
    break;
  }
}
void Connection::WriteBlocking() {
  Errif(write(conn_socket_->GetFd(), output_buffer_->GetData(), output_buffer_->GetSize()) == -1, "write error");
}
void Connection::WriteNonBlocking() {
  Errif(write(conn_socket_->GetFd(), output_buffer_->GetData(), output_buffer_->GetSize()) == -1, "write error");
}
void Connection::ReadKeyBoard() {
  std::string buf;
  std::getline(std::cin, buf);
  input_buffer_->Input(buf.c_str());
}

const char *Connection::ReadInputBuffer() const { return input_buffer_->GetData(); }

void Connection::SetOutput(const char *data) {
  output_buffer_->Clear();
  output_buffer_->Input(data);
}

void Connection::RemoveConnection() {
  conn_channel_->RemoveInEpoll();
  remove_(conn_socket_);
}  // 关闭进行中的读写操作，并移除连接

void Connection::Close() {  // 关闭端口连接
  close(conn_socket_->GetFd());
}

void Connection::SetState(Connection::State state) { Connection::state_ = state; }

Connection::State Connection::GetState() const { return Connection::state_; }
