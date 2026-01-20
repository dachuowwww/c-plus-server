#include "Connection.h"
#include <sys/sendfile.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "Buffer.h"
#include "Channel.h"
#include "Error.h"
#include "EventLoop.h"
#include "HttpContext.h"
#include "Logger.h"
#include "Socket.h"
using std::cout;
using std::endl;
using std::function;

const int SERV_BUFFER = 1024;
Connection::Connection(EventLoop *loop, int cln_fd) : loop_(loop) {
  conn_socket_ = std::make_unique<Socket>(cln_fd);
  input_buffer_ = std::make_unique<Buffer>();
  output_buffer_ = std::make_unique<Buffer>();
  state_ = State::Connected;  // 默认已连接才可以进行客户端的读写操作
  if (loop_ != nullptr) {
    conn_socket_->SetNonBlocking();

    conn_channel_ = std::make_unique<Channel>(loop_, cln_fd);
    conn_channel_->SetReadCallback([this]() { this->ListenClientMessage(); });  // 命名空间调用可省略，取地址不行。
    conn_channel_->SetWriteCallback([this]() { this->SendClientMessage(); });
  }
  context_ = std::make_unique<HttpContext>();
}
Connection::~Connection() = default;

void Connection::ConnectionEstablished() {
  conn_channel_->Tie(
      shared_from_this());  // 构造函数不能写shared_from_this，因为还没构造完毕。对象内部创建一个指向自己的共享指针 +1
  conn_channel_->EnableReading();
  connnect_func_(shared_from_this());
}

void Connection::ConnectionDestructor() {
  int fd = conn_channel_->GetFd();
  conn_channel_->RemoveInEpoll();
  LOG_INFO << "Client fd " << fd << " has been ultimately removed";
}  // -1

void Connection::SetRemoveConnection(function<void(const std::shared_ptr<Connection> &conn)> &&cb) {
  remove_ = std::move(cb);
}

void Connection::RemoveConnection() {
  remove_(shared_from_this());  // 不会泄露但是很危险 use after free
}  // 关闭进行中的读写操作，并移除连接

bool Connection::IsInEpoll() const { return conn_channel_->IfInEpoll(); }
EventLoop *Connection::GetLoop() const { return loop_; }
int Connection::GetFd() const { return conn_socket_->GetFd(); }
void Connection::SetET() { conn_channel_->UseET(); }

// void Connection::EnableReading() {  }
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

void Connection::SetHandleReadFunc(function<void(const std::shared_ptr<Connection> &conn)> cb) {
  handle_read_func_ = std::move(cb);
  // conn_channel_->SetReadCallback([this]() { handle_read_func_(this); });
}

void Connection::SetConnect(function<void(const std::shared_ptr<Connection> &conn)> cb) {
  connnect_func_ = std::move(cb);
}

void Connection::ListenClientMessage() {
  Read();
  if (state_ != State::Connected) {
    return;
  }
  handle_read_func_(shared_from_this());
}
void Connection::SendClientMessage() {
  Write();
  if (state_ != State::Connected) {
    return;
  }
}

void Connection::Send(const std::string &data) { Send(data.c_str(), data.size()); }

void Connection::Send(const char *data) { Send(data, static_cast<int>(strlen(data))); }

void Connection::Send(const char *data, int size) {
  int remaining = size;
  int send_size = 0;
  if (output_buffer_->ReadableBytes() == 0) {
    send_size = write(conn_socket_->GetFd(), data, remaining);
    if (send_size >= 0) {
      remaining -= send_size;
    } else if (send_size == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
      cout << "buffer full, wait for writing" << endl;
      send_size = 0;
    } else {
      cout << "other write error" << endl;
      LOG_ERROR << "TcpConnection::Send - TcpConnection Send ERROR";
      SetState(State::Closed);
      RemoveConnection();
      return;
    }
  }
  if (remaining > 0) {
    output_buffer_->Append(data + send_size, remaining);
    conn_channel_->EnableWriting();
  }
}

void Connection::SendFile(int fd, int size) {
  ssize_t data_size = static_cast<ssize_t>(size);
  ssize_t send_size = 0;
  while (send_size < data_size) {
    ssize_t bytes_write = sendfile(GetFd(), fd, (off_t *)&send_size, data_size - send_size);
    if (bytes_write > 0) {
      send_size += bytes_write;
    } else if (bytes_write == -1) {
      if (errno == EINTR || errno == EAGAIN || (errno == EWOULDBLOCK)) {
        continue;
      }
      break;
    } else {
      LOG_ERROR << "Connection::SendFile - TcpConnection Send ERROR";
      break;
    }
  }
}

void Connection::Read() {
  if (state_ != State::Connected) {
    LOG_ERROR << "Read not connected";
    RemoveConnection();
    return;
  }
  input_buffer_->RetreiveAll();  // 不能去，因为非租塞使用append处理数据
  if (conn_socket_->IsNonBlocking()) {
    ReadNonBlocking();  // 程序进行中进行连接判断，如果连接正常，进行阻塞IO读取
  } else {
    ReadBlocking();
  }
}

void Connection::Write() {  // 为未发完的信息服务
  if (state_ != State::Connected) {
    LOG_ERROR << "Write not connected";
    RemoveConnection();
    return;
  }
  if (conn_socket_->IsNonBlocking()) {
    WriteNonBlocking();
  } else {
    WriteBlocking();
  }
  // output_buffer_->Clear();
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
      // cout << "finish reading once, errno: " << errno << endl;
      UpdateTimeStamp();
      break;
    }
    if (bytes_read == 0) {  // EOF，对方断开连接
      LOG_ERROR << "Read EOF, fd " << conn_socket_->GetFd() << " disconnected";
      SetState(State::Closed);
      RemoveConnection();
      break;
    }
    LOG_ERROR << "other read error";
    SetState(State::Closed);
    RemoveConnection();
    break;
  }
}

void Connection::ReadBlocking() {
  // std::cout << "blocking read" << std::endl;
  char buf[SERV_BUFFER];
  memset(buf, 0, sizeof(buf));
  int had_read = read(conn_socket_->GetFd(), buf, sizeof(buf));
  if (had_read > 0) {
    input_buffer_->Append(buf, had_read);
    UpdateTimeStamp();
    // input_buffer_->Append("\0");
  } else if (had_read == 0) {  // EOF，对方断开连接
    LOG_ERROR << "Read EOF, fd " << conn_socket_->GetFd() << " disconnected";
    SetState(State::Closed);
    if (remove_) {
      RemoveConnection();
    }
  }
}

void Connection::WriteNonBlocking() {
  // std::cout << "non-blocking write" << std::endl;
  int bytes_to_write = output_buffer_->ReadableBytes();
  if (bytes_to_write == 0) {
    conn_channel_->DisableWriting();
    return;
  }
  // std::string buf = output_buffer_->RetreiveAllAsString();
  int bytes_written = 0;

  int fd = conn_socket_->GetFd();
  bytes_written = write(fd, output_buffer_->Peek(), bytes_to_write);
  if (bytes_written > 0) {
    output_buffer_->Retreive(static_cast<int>(bytes_written));
  } else if (bytes_written == -1 &&
             ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {  // 非阻塞IO，这个条件表示缓冲区已满，需要等待
    cout << "buffer full, wait for writing" << endl;
  } else {
    LOG_ERROR << "other write error";
    SetState(State::Closed);
    RemoveConnection();
  }
}
void Connection::WriteBlocking() {
  // std::cout << "blocking write" << std::endl;
  int had_write = 0;
  size_t bytes_to_write = output_buffer_->ReadableBytes();
  while (true) {
    had_write = write(conn_socket_->GetFd(), output_buffer_->Peek(), bytes_to_write);
    if (had_write > 0) {
      // std::cout << "write " << n << " bytes , content: " << output_buffer_->GetData() << std::endl;
      bytes_to_write -= had_write;
      output_buffer_->Retreive(static_cast<int>(had_write));
      if (bytes_to_write == 0) {
        break;
      }
    } else if (had_write == -1 && errno == EINTR) {  // 对方正常中断、继续写入
      cout << "continue writing" << endl;
      continue;
    } else {
      LOG_ERROR << "other write error";
      SetState(State::Closed);
      if (remove_) {
        RemoveConnection();
      }
      break;
    }
  }
}

void Connection::KeyBoardToOutput() { output_buffer_->AppendKeyBoard(); }

std::string Connection::ReadInputBuffer() const { return input_buffer_->PeekAllString(); }

int Connection::ReadInputBufferSize() const { return input_buffer_->ReadableBytes(); }

std::string Connection::ReadOutputBuffer() const { return output_buffer_->PeekAllString(); }

int Connection::ReadOutputBufferSize() const { return input_buffer_->ReadableBytes(); }

std::string Connection::RetriveInputBuffer() const { return input_buffer_->RetreiveAllAsString(); }

void Connection::SetOutput(const char *data) { output_buffer_->Append(data); }

void Connection::SetState(State state) { state_ = state; }  // 这种类型（enum、int、struct 无指针成员）完全不需要 move。

Connection::State Connection::GetState() const { return state_; }

HttpContext *Connection::GetContext() const { return context_.get(); }

void Connection::UpdateTimeStamp() { conn_time_ = TimeStamp::Now(); }

TimeStamp Connection::GetTimeStamp() const { return conn_time_; }
