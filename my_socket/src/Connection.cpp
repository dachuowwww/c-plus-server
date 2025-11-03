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
Connection::Connection(shared_ptr<EventLoop> loop, shared_ptr<TCPSocket> socket)
    : loop_(std::move(loop)), conn_socket_(socket), input_buffer_(new Buffer()), output_buffer_(new Buffer()), connected_(true) {
  conn_channel_.reset(new Channel(loop_, conn_socket_->GetFd()));
  function<void()> cb1 = std::bind(&Connection::Echo, this);
  conn_channel_->SetReadCallback(cb1);
  function<void()> cb2 = std::bind(&Connection::RemoveConnection, this);
  conn_channel_->SetCloseCallback(cb2);
  // conn_channel->SetThreadPool(true);
}

Connection::~Connection() = default;

void Connection::SetRemoveConnection(const function<void(shared_ptr<TCPSocket>)> &cb) { remove_ = cb; }
bool Connection::IsInEpoll() const { return conn_channel_->IfInEpoll(); }
int Connection::GetFd() const { return conn_channel_->GetFd(); }
void Connection::EnableReading() { conn_channel_->EnableReading(); }
void Connection::Echo() {
  if (!IsConnected()) {
    cout << "client fd " << conn_socket_->GetFd() << " disconnected" << endl;
    return;
  }
  char buf[SERV_BUFFER];
  while (true) {
    bzero(&buf, sizeof(buf));
    ssize_t bytes_read = read(conn_socket_->GetFd(), buf, sizeof(buf));
    if (bytes_read > 0) {
      input_buffer_->Append(buf, bytes_read);
      // cout<<bytes_read<<" bytes message from client fd "<<conn_socket->GetFd()<<" : "<<buf<<endl;
      // write(conn_socket->GetFd(), buf, bytes_read);
    } else if (bytes_read == -1 && errno == EINTR) {  // 客户端正常中断、继续读取
      cout << "continue reading" << endl;
      continue;
    } else if (bytes_read == -1 &&
               ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {  // 非阻塞IO，这个条件表示数据全部读取完毕
      cout << "finish reading once, errno: " << errno << endl;
      if (input_buffer_->GetSize() > 0) {
        cout << "message from client fd " << conn_socket_->GetFd() << " : " << input_buffer_->ReadAll() << endl;
        Errif(write(conn_socket_->GetFd(), input_buffer_->ReadAll(), input_buffer_->GetSize()) == -1, "write error");
        input_buffer_->Clear();
      }
      break;
    } else if (bytes_read == 0) {  // EOF，客户端断开连接
      cout << "EOF, client fd " << conn_socket_->GetFd() << " disconnected" << endl;
      RemoveConnection();
      break;
    } else {
      perror("read error");
      RemoveConnection();
      break;
    }
  }
}

void Connection::RemoveConnection() { remove_(conn_socket_); }

bool Connection::IsConnected() const { return connected_.load(); }
