#include "HttpServer.h"
#include <arpa/inet.h>
#include <algorithm>
#include <cctype>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include "Connection.h"
#include "Error.h"
#include "EventLoop.h"
#include "HttpContext.h"
#include "HttpResponse.h"
#include "Metrics.h"
#include "Server.h"
#include "TimeStamp.h"

// namespace {
// int GetRequestLength(const std::string &data) {
//   size_t header_end = data.find("\r\n\r\n");
//   if (header_end == std::string::npos) {
//     return 0;
//   }
//   size_t body_start = header_end + 4;
//   std::string headers = data.substr(0, header_end);
//   std::string headers_lower = headers;
//   std::transform(headers_lower.begin(), headers_lower.end(), headers_lower.begin(),
//                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

//   const std::string key = "content-length:";
//   size_t pos = headers_lower.find(key);
//   if (pos == std::string::npos) {
//     return static_cast<int>(body_start);
//   }
//   pos += key.size();
//   while (pos < headers_lower.size() && std::isspace(static_cast<unsigned char>(headers_lower[pos]))) {
//     ++pos;
//   }
//   size_t start = pos;
//   while (pos < headers_lower.size() && std::isdigit(static_cast<unsigned char>(headers_lower[pos]))) {
//     ++pos;
//   }
//   if (start == pos) {
//     return static_cast<int>(body_start);
//   }
//   size_t content_length = 0;
//   for (size_t i = start; i < pos; ++i) {
//     content_length = content_length * 10 + static_cast<size_t>(headers_lower[i] - '0');
//   }
//   if (data.size() < body_start + content_length) {
//     return 0;
//   }
//   return static_cast<int>(body_start + content_length);
// }
namespace {
struct CachedFile {
  std::string data;
  size_t size = 0;
};

constexpr size_t kFileCacheMaxEntries = 128;             // 最大缓存文件数量
constexpr size_t kFileCacheMaxBytes = 32 * 1024 * 1024;  // 最大缓存总字节数（32MB）

std::mutex g_file_cache_mutex;
std::unordered_map<std::string, CachedFile> g_file_cache;
std::deque<std::string> g_file_cache_order;  // 用于记录缓存文件的访问顺序，便于实现LRU淘汰策略
size_t g_file_cache_bytes = 0;               // 记录当前缓存的总字节数，便于判断是否超过限制

void EvictCacheIfNeeded(size_t incoming_size) {
  while (!g_file_cache_order.empty() &&
         (g_file_cache.size() >= kFileCacheMaxEntries || g_file_cache_bytes + incoming_size > kFileCacheMaxBytes)) {
    const std::string &key = g_file_cache_order.front();
    g_file_cache_order.pop_front();
    auto it = g_file_cache.find(key);
    if (it != g_file_cache.end()) {
      g_file_cache_bytes -= it->second.size;
      g_file_cache.erase(it);
    }
  }
}
}  // namespace

std::string HttpServer::ReadFileCached(const std::string &filename) {  // 简单缓存/文件内容池
  {
    std::lock_guard<std::mutex> lock(g_file_cache_mutex);
    auto it = g_file_cache.find(filename);
    if (it != g_file_cache.end()) {
      return it->second.data;
    }
  }

  std::ifstream is(filename.c_str(), std::ifstream::in | std::ifstream::binary);
  if (!is.is_open()) {
    Errif(true, "ReadFile: open file failed!");
    return "";
  }
  is.seekg(0, std::ifstream::end);
  std::streamoff length = is.tellg();
  if (length <= 0) {
    return "";
  }
  is.seekg(0, std::ifstream::beg);
  std::string content;
  content.resize(static_cast<size_t>(length));
  is.read(content.data(), length);
  is.close();
  {
    std::lock_guard<std::mutex> lock(g_file_cache_mutex);
    auto it = g_file_cache.find(filename);
    if (it != g_file_cache.end()) {
      return it->second.data;
    }
    const size_t size = content.size();
    if (size <= kFileCacheMaxBytes) {
      EvictCacheIfNeeded(size);
      g_file_cache_order.push_back(filename);
      g_file_cache_bytes += size;
      g_file_cache.emplace(filename, CachedFile{content, size});
    }
  }

  return content;
}

HttpServer::HttpServer(const char *ip, uint16_t port) {
  loop_ = std::make_unique<EventLoop>();
  server_ = std::make_unique<Server>(loop_.get(), ip, port);
  server_->OnConnect([this](const std::shared_ptr<Connection> &conn) { OnConnnectCallback(conn); });
  LOG_INFO << "HttpServer Listening on [ " << ip << ":" << port << " ]";
}
HttpServer::~HttpServer() = default;

void HttpServer::Start() { loop_->Loop(); }

coro::DetachedTask HttpServer::HandleHttpSession(
    std::shared_ptr<Connection> conn) {  // 这里不能用const引用，因为co_await会导致函数挂起，期间conn可能被修改
  while (conn->GetState() == Connection::State::Connected) {
    co_await conn->WaitReadable();
    if (conn->GetState() != Connection::State::Connected) {
      co_return;
    }
    OnHttpRequest(conn);
  }
}

void HttpServer::OnConnnectCallback(const std::shared_ptr<Connection> &conn) {
  int clnt_fd = conn->GetFd();
  struct sockaddr_in cln_addr {};
  socklen_t cln_addrlength = sizeof(cln_addr);
  getpeername(clnt_fd, (struct sockaddr *)&cln_addr, &cln_addrlength);
  // std::cout << "new client fd " << conn->GetFd() << "! IP: " << inet_ntoa(cln_addr.sin_addr)
  //           << " Port:" << ntohs(cln_addr.sin_port) << std::endl;
  LOG_INFO << "HttpServer::OnNewConnection : Add connection "
           << "[ fd#" << clnt_fd << " ]"
           << " from " << inet_ntoa(cln_addr.sin_addr) << ":" << ntohs(cln_addr.sin_port);
  if (auto_shutdown_) {
    loop_->RunAfter(AUTOCLOSETIMEOUT, bind(&HttpServer::OverTime, this,
                                           std::weak_ptr<Connection>(conn)));  // 开启其他线程Loop会造成线程冲突
  }
  if (use_coroutine_) {
    HandleHttpSession(conn);
  }
}

void HttpServer::OnHttpRequest(const std::shared_ptr<Connection> &conn) {
  int size = conn->ReadInputBufferSize();  // 提前获取大小，retrive会清空
  if (size == 0) {
    return;
  }
  HttpContext *context = conn->GetContext();
  Metrics::OnRequest();

  if (!context->ParaseRequest(conn->RetriveInputBuffer().c_str(), size)) {
    Metrics::OnParseError();
    conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->SetState(Connection::State::Closed);
    if (conn->IsInEpoll()) {
      conn->RemoveConnection();
    }
  } else {
    LOG_INFO << context->GetHttpRequest()->GetURL()
             << ", request successfully. Method:" << context->GetHttpRequest()->GetMethodString();
    if (conn->GetContext()->IsComplete()) {
      OnHttpReponse(conn);
      context->ResetState();
    }
  }
}

void HttpServer::OnHttpReponse(const std::shared_ptr<Connection> &conn) {
  HttpRequest const *request = conn->GetContext()->GetHttpRequest();
  bool close = request->GetHeader("Connection") == "Close" ||
               (request->GetVersionString() == "Http1.0" && request->GetHeader("Connection") != "Keep-Alive");
  if (request->GetHeader("Content-Type").find("multipart/form-data") != std::string::npos) {  // 处理文件上传
    FileUpload(request);
  }
  auto response = std::make_unique<HttpResponse>(close);
  http_call_back_(*request, response.get());
  // std::cout << response->GetResponse()<< std::endl;
  if (response->GetBodyType() == "HTML_TYPE") {
    conn->Send(response->GetResponse());  // 带图片时不能用strlen
    // std::cout << response->GetResponse() << std::endl;
  } else {
    conn->Send(response->GetPreBody());
    conn->SendFile(response->GetFileId(), response->GetContentLength());
  }

  if (response->IfClose()) {
    conn->SetState(Connection::State::Closed);
    conn->RemoveConnection();
  }
}

void HttpServer::SetMessageCallBack(std::function<void(const std::shared_ptr<Connection> &conn)> &&cb) {
  use_coroutine_ = false;
  server_->OnMessage(std::move(cb));
}

void HttpServer::SetHttpResponseCallBack(std::function<void(const HttpRequest &request, HttpResponse *response)> &&cb) {
  http_call_back_ = std::move(cb);
  if (!use_coroutine_) {
    server_->OnMessage([this](const std::shared_ptr<Connection> &conn) {
      OnHttpRequest(conn);
    });  // 成员变量函数必须明确对象(无关是否需要本对象元素，类就要明确this)}
  }
}

void HttpServer::OnTimerEvery(double interval, std::function<void()> &&cb) { loop_->RunEvery(interval, std::move(cb)); }

void HttpServer::OverTime(const std::weak_ptr<Connection> &conn) {
  auto conn_ptr = conn.lock();
  if (conn_ptr) {
    if (TimeStamp::AddTime(conn_ptr->GetTimeStamp(), AUTOCLOSETIMEOUT) < TimeStamp::Now()) {
      conn_ptr->RemoveConnection();
    } else {
      loop_->RunAfter(
          (double)(AUTOCLOSETIMEOUT - (TimeStamp::Now().Time() - static_cast<double>(conn_ptr->GetTimeStamp().Time())) /
                                          MICROSECOND_2_SECOND) +
              1.0,
          bind(&HttpServer::OverTime, this, conn));
    }
  }
}
