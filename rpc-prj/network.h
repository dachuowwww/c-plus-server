#pragma once

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <iostream>

// 网络事件类型
enum class EventType {
    READ = EPOLLIN,
    WRITE = EPOLLOUT,
    ERROR = EPOLLERR | EPOLLHUP
};

// 连接状态
enum class ConnectionState {
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    ERROR_STATE
};

// 前向声明
class EventLoop;
class Connection;

// 事件回调函数类型
using EventCallback = std::function<void(int fd, EventType event)>;
using ConnectionCallback = std::function<void(std::shared_ptr<Connection>)>;
using MessageCallback = std::function<void(std::shared_ptr<Connection>, const std::string&)>;

// 连接类
class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(int fd, EventLoop* loop);
    ~Connection();
    
    void Send(const std::string& data);
    void Close();
    
    int GetFd() const { return fd_; }
    ConnectionState GetState() const { return state_; }
    
    void SetMessageCallback(MessageCallback cb) { message_callback_ = cb; }
    void SetCloseCallback(ConnectionCallback cb) { close_callback_ = cb; }
    
    // 处理读事件
    void HandleRead();
    // 处理写事件
    void HandleWrite();
    // 处理错误事件
    void HandleError();
    
private:
    int fd_;
    EventLoop* loop_;
    ConnectionState state_;
    
    std::string read_buffer_;
    std::string write_buffer_;
    std::mutex write_mutex_;
    
    MessageCallback message_callback_;
    ConnectionCallback close_callback_;
    
    void SetNonBlocking();
    ssize_t ReadFromSocket();
    ssize_t WriteToSocket();
};

// 事件循环类 - Reactor模式的核心
class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    
    void Run();
    void Stop();
    
    void AddEvent(int fd, EventType events, EventCallback callback);
    void ModifyEvent(int fd, EventType events);
    void RemoveEvent(int fd);
    
    void RunInLoop(std::function<void()> task);
    
private:
    int epoll_fd_;
    bool running_;
    std::thread::id loop_thread_id_;
    
    std::unordered_map<int, EventCallback> event_callbacks_;
    
    // 任务队列用于线程间通信
    std::queue<std::function<void()>> pending_tasks_;
    std::mutex task_mutex_;
    int wakeup_fd_[2];  // 用于唤醒事件循环的管道
    
    void HandleWakeup();
    void WakeupLoop();
    bool IsInLoopThread() const;
};

// TCP服务器类
class TcpServer {
public:
    TcpServer(EventLoop* loop, const std::string& ip, int port);
    ~TcpServer();
    
    void Start();
    void Stop();
    
    void SetConnectionCallback(ConnectionCallback cb) { connection_callback_ = cb; }
    void SetMessageCallback(MessageCallback cb) { message_callback_ = cb; }
    
private:
    EventLoop* loop_;
    std::string ip_;
    int port_;
    int listen_fd_;
    bool started_;
    
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;
    
    void HandleNewConnection();
    void HandleConnectionClose(std::shared_ptr<Connection> conn);
    
    int CreateListenSocket();
    void SetReuseAddr(int fd);
};

// TCP客户端类
class TcpClient {
public:
    TcpClient(EventLoop* loop);
    ~TcpClient();
    
    void Connect(const std::string& ip, int port);
    void Disconnect();
    
    void Send(const std::string& data);
    
    void SetConnectionCallback(ConnectionCallback cb) { connection_callback_ = cb; }
    void SetMessageCallback(MessageCallback cb) { message_callback_ = cb; }
    
private:
    EventLoop* loop_;
    std::shared_ptr<Connection> connection_;
    
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    
    void HandleConnection(std::shared_ptr<Connection> conn);
}; 