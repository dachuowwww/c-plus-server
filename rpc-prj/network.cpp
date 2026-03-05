#include "network.h"

// Connection 实现
Connection::Connection(int fd, EventLoop* loop) 
    : fd_(fd), loop_(loop), state_(ConnectionState::CONNECTED) {
    SetNonBlocking();
}

Connection::~Connection() {
    Close();
}

void Connection::Send(const std::string& data) {
    if (state_ != ConnectionState::CONNECTED) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(write_mutex_);
    write_buffer_.append(data);
    
    // 注册写事件
    loop_->ModifyEvent(fd_, static_cast<EventType>(EPOLLIN | EPOLLOUT));
}

void Connection::Close() {
    if (state_ == ConnectionState::DISCONNECTED) {
        return;
    }
    
    state_ = ConnectionState::DISCONNECTED;
    loop_->RemoveEvent(fd_);
    ::close(fd_);
}

void Connection::HandleRead() {
    ssize_t n = ReadFromSocket();
    if (n > 0) {
        if (message_callback_) {
            message_callback_(shared_from_this(), read_buffer_);
            read_buffer_.clear();
        }
    } else if (n == 0) {
        Close();
    } else {
        HandleError();
    }
}

void Connection::HandleWrite() {
    std::lock_guard<std::mutex> lock(write_mutex_);
    if (!write_buffer_.empty()) {
        ssize_t written = WriteToSocket();
        if (written < 0) {
            HandleError();
            return;
        }
        
        if (write_buffer_.empty()) {
            loop_->ModifyEvent(fd_, EventType::READ);
        }
    }
}

void Connection::HandleError() {
    state_ = ConnectionState::ERROR_STATE;
    Close();
}

void Connection::SetNonBlocking() {
    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
}

ssize_t Connection::ReadFromSocket() {
    char buffer[4096];
    ssize_t n = ::read(fd_, buffer, sizeof(buffer));
    if (n > 0) {
        read_buffer_.append(buffer, n);
    }
    return n;
}

ssize_t Connection::WriteToSocket() {
    ssize_t written = ::write(fd_, write_buffer_.data(), write_buffer_.size());
    if (written > 0) {
        write_buffer_.erase(0, written);
    }
    return written;
}

// EventLoop 实现
EventLoop::EventLoop() : running_(false) {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0) {
        throw std::runtime_error("Failed to create epoll fd");
    }
    
    if (pipe(wakeup_fd_) < 0) {
        throw std::runtime_error("Failed to create wakeup pipe");
    }
    
    fcntl(wakeup_fd_[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeup_fd_[1], F_SETFL, O_NONBLOCK);
    
    AddEvent(wakeup_fd_[0], EventType::READ, 
             [this](int, EventType) { HandleWakeup(); });
}

EventLoop::~EventLoop() {
    Stop();
    ::close(epoll_fd_);
    ::close(wakeup_fd_[0]);
    ::close(wakeup_fd_[1]);
}

void EventLoop::Run() {
    running_ = true;
    loop_thread_id_ = std::this_thread::get_id();
    
    const int kMaxEvents = 1024;
    struct epoll_event events[kMaxEvents];
    
    while (running_) {
        int num_events = epoll_wait(epoll_fd_, events, kMaxEvents, 1000);
        
        if (num_events < 0) {
            if (errno == EINTR) continue;
            break;
        }
        
        for (int i = 0; i < num_events; ++i) {
            int fd = events[i].data.fd;
            uint32_t revents = events[i].events;
            
            auto it = event_callbacks_.find(fd);
            if (it != event_callbacks_.end()) {
                EventType event_type = static_cast<EventType>(revents);
                it->second(fd, event_type);
            }
        }
        
        std::queue<std::function<void()>> tasks;
        {
            std::lock_guard<std::mutex> lock(task_mutex_);
            tasks.swap(pending_tasks_);
        }
        
        while (!tasks.empty()) {
            tasks.front()();
            tasks.pop();
        }
    }
}

void EventLoop::Stop() {
    running_ = false;
    WakeupLoop();
}

void EventLoop::AddEvent(int fd, EventType events, EventCallback callback) {
    struct epoll_event ev;
    ev.events = static_cast<uint32_t>(events);
    ev.data.fd = fd;
    
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    event_callbacks_[fd] = callback;
}

void EventLoop::ModifyEvent(int fd, EventType events) {
    struct epoll_event ev;
    ev.events = static_cast<uint32_t>(events);
    ev.data.fd = fd;
    
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

void EventLoop::RemoveEvent(int fd) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    event_callbacks_.erase(fd);
}

void EventLoop::RunInLoop(std::function<void()> task) {
    if (IsInLoopThread()) {
        task();
    } else {
        {
            std::lock_guard<std::mutex> lock(task_mutex_);
            pending_tasks_.push(task);
        }
        WakeupLoop();
    }
}

void EventLoop::HandleWakeup() {
    char buffer[64];
    ::read(wakeup_fd_[0], buffer, sizeof(buffer));
}

void EventLoop::WakeupLoop() {
    char data = 1;
    ::write(wakeup_fd_[1], &data, 1);
}

bool EventLoop::IsInLoopThread() const {
    return loop_thread_id_ == std::this_thread::get_id();
}

// TcpServer 实现
TcpServer::TcpServer(EventLoop* loop, const std::string& ip, int port)
    : loop_(loop), ip_(ip), port_(port), listen_fd_(-1), started_(false) {
}

TcpServer::~TcpServer() {
    Stop();
}

void TcpServer::Start() {
    if (started_) return;
    
    listen_fd_ = CreateListenSocket();
    if (listen_fd_ < 0) {
        throw std::runtime_error("Failed to create listen socket");
    }
    
    loop_->AddEvent(listen_fd_, EventType::READ,
                    [this](int, EventType) { HandleNewConnection(); });
    
    started_ = true;
    std::cout << "Server started on " << ip_ << ":" << port_ << std::endl;
}

void TcpServer::Stop() {
    if (!started_) return;
    
    started_ = false;
    if (listen_fd_ >= 0) {
        loop_->RemoveEvent(listen_fd_);
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
    
    for (auto& pair : connections_) {
        pair.second->Close();
    }
    connections_.clear();
}

void TcpServer::HandleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) return;
    
    auto conn = std::make_shared<Connection>(client_fd, loop_);
    conn->SetMessageCallback(message_callback_);
    
    connections_[client_fd] = conn;
    
    loop_->AddEvent(client_fd, EventType::READ,
                    [conn](int, EventType event) {
                        if (event == EventType::READ) {
                            conn->HandleRead();
                        } else if (event == EventType::WRITE) {
                            conn->HandleWrite();
                        } else {
                            conn->HandleError();
                        }
                    });
    
    if (connection_callback_) {
        connection_callback_(conn);
    }
}

void TcpServer::HandleConnectionClose(std::shared_ptr<Connection> conn) {
    connections_.erase(conn->GetFd());
}

int TcpServer::CreateListenSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        return -1;
    }
    
    SetReuseAddr(fd);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    
    // 使用INADDR_ANY来监听所有接口
    if (ip_ == "0.0.0.0" || ip_ == "127.0.0.1") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        addr.sin_addr.s_addr = inet_addr(ip_.c_str());
    }
    
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind() failed on " << ip_ << ":" << port_ 
                  << " - " << strerror(errno) << std::endl;
        ::close(fd);
        return -1;
    }
    
    if (listen(fd, 128) < 0) {
        std::cerr << "listen() failed: " << strerror(errno) << std::endl;
        ::close(fd);
        return -1;
    }
    
    return fd;
}

void TcpServer::SetReuseAddr(int fd) {
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

// TcpClient 实现
TcpClient::TcpClient(EventLoop* loop) : loop_(loop) {
}

TcpClient::~TcpClient() {
    Disconnect();
}

void TcpClient::Connect(const std::string& ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to " << ip << ":" << port 
                  << " - " << strerror(errno) << std::endl;
        ::close(fd);
        return;
    }
    
    connection_ = std::make_shared<Connection>(fd, loop_);
    connection_->SetMessageCallback(message_callback_);
    
    // 监听连接的读事件
    loop_->AddEvent(fd, EventType::READ,
                    [this](int, EventType event) {
                        if (event == EventType::READ) {
                            connection_->HandleRead();
                        } else if (event == EventType::WRITE) {
                            connection_->HandleWrite();
                        } else {
                            connection_->HandleError();
                        }
                    });
    
    if (connection_callback_) {
        connection_callback_(connection_);
    }
    
    std::cout << "Connected to " << ip << ":" << port << std::endl;
}

void TcpClient::Disconnect() {
    if (connection_) {
        connection_->Close();
        connection_.reset();
    }
}

void TcpClient::Send(const std::string& data) {
    if (connection_) {
        connection_->Send(data);
    }
}

void TcpClient::HandleConnection(std::shared_ptr<Connection> conn) {
    connection_ = conn;
}

 