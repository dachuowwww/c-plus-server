#include "HttpServer.h"
#include <arpa/inet.h>
#include <algorithm>
#include <cctype>
#include <deque>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>  // 引入json库
#include <unordered_map>
#include "Connection.h"
#include "Error.h"
#include "EventLoop.h"
#include "HttpContext.h"
#include "HttpResponse.h"
#include "Logger.h"
#include "Macro.h"
#include "Metrics.h"
#include "Server.h"
#include "ThreadPool.h"
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
using Json = nlohmann::json;
using RpcHandler = std::function<bool(const Json &params, Json *result)>;
using RpcCode = HttpResponse::RpcCode;
constexpr int kRpcTimeoutMs = 2000;

Json MakeRespJson(int id, bool ok, HttpResponse::RpcCode code, const std::string &message, const Json &result) {
  Json resp;
  resp["id"] = id;
  resp["ok"] = ok;
  resp["code"] = static_cast<int>(code);
  resp["message"] = message;
  resp["result"] = ok ? result : Json(nullptr);
  return resp;
}

std::string BuildRpcHttpResponse(const Json &resp, bool close, HttpResponse::HttpStatusCode status) {
  HttpResponse response(close);
  response.SetStatusCode(status);
  response.SetStatusMessage(status == HttpResponse::HttpStatusCode::K400BADREQUEST ? "BAD_RESQUEST" : "OK");
  response.SetContentType("application/json; charset=UTF-8");
  response.SetResponseBody(resp.dump());
  return response.GetResponse();
}

void SendRpcRawResponse(const std::shared_ptr<Connection> &conn, std::string response, bool close) {
  if (conn->GetState() != Connection::State::Connected) {
    return;
  }
  conn->Send(response);
  if (close) {
    conn->SetState(Connection::State::Closed);
    conn->RemoveConnection();
  }
}

const std::unordered_map<std::string, RpcHandler> rpc_handlers = {{"Echo", [](const Json &params, Json *result) {
                                                                     *result = params;
                                                                     return true;
                                                                   }}};
}  // namespace
// void HttpServer::HandleRpcRequest(const HttpRequest &request, HttpResponse *response) {  // rpc没有线程池的话执行此处
//   Json req;
//   try {
//     req = Json::parse(request.GetBody());  // 解析
//   } catch (const Json::parse_error &) {
//     Json resp = MakeRespJson(0, false, RpcCode::KRPCBADJSON, "bad json", Json{});
//     response->SetStatusCode(HttpResponse::HttpStatusCode::K400BADREQUEST);
//     response->SetStatusMessage("BAD_RESQUEST");
//     response->SetResponseBody(resp.dump());  // 对象序列化
//     response->SetContentType("application/json; charset=UTF-8");
//     return;
//   }

//   Json resp;
//   if (!req.contains("id") || !req["id"].is_number_integer()) {
//     resp = MakeRespJson(0, false, RpcCode::KRPCBADPARAMS, "bad params", Json{});
//   } else if (!req.contains("method") || !req["method"].is_string() || !req.contains("params") ||
//              !req["params"].is_object()) {
//     resp = MakeRespJson(req.value("id", 0), false, RpcCode::KRPCBADPARAMS, "bad params", Json{});
//   } else {
//     int id = req.value("id", 0);
//     std::string method = req.value("method", "");
//     Json params = req.value("params", Json::object());

//     Json result = Json::object();
//     auto it = rpc_handlers.find(method);
//     if (it == rpc_handlers.end()) {
//       resp = MakeRespJson(id, false, RpcCode::KRPCMETHODNOTFOUND, "method not found", Json{});
//     } else {
//       bool ok = false;
//       try {
//         ok = it->second(params, &result);
//       } catch (...) {
//         ok = false;
//       }
//       if (ok) {
//         resp = MakeRespJson(id, true, RpcCode::KRPCOK, "OK", result);
//       } else {
//         resp = MakeRespJson(id, false, RpcCode::KRPCINTERNALERROR, "internal error", Json{});
//       }
//     }
//   }

//   response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
//   response->SetStatusMessage("OK");
//   response->SetResponseBody(resp.dump());  // 对象序列化
//   response->SetContentType("application/json; charset=UTF-8");
// }
void HttpServer::EnableRpcFlowPool(unsigned int size) {
  if (size == 0) {
    size = 1;
  }
  rpc_flow_pool_ = std::make_unique<ThreadPool>(size);
  use_rpc_flow_pool_ = true;
}

void HttpServer::DisableRpcFlowPool() {
  use_rpc_flow_pool_ = false;
  rpc_flow_pool_.reset();
}

//////////////////////////////// 文件缓存池
namespace {
struct CachedFile {
  std::string data;
  size_t size = 0;
};

constexpr size_t kFileCacheMaxEntries = 128;             // 最大缓存文件数量
constexpr size_t kFileCacheMaxBytes = 32 * 1024 * 1024;  // 最大缓存总字节数（32MB）

std::mutex g_file_cache_mutex;
std::unordered_map<std::string, CachedFile> g_file_cache;
std::deque<std::string> g_file_cache_order;  // 用于记录缓存文件的访问顺序，便于实现FIFO淘汰策略
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
/////////////////////////////////
HttpServer::HttpServer(const char *ip, uint16_t port) {
  loop_ = std::make_unique<EventLoop>();
  server_ = std::make_unique<Server>(loop_.get(), ip, port);
  server_->OnConnect([this](const std::shared_ptr<Connection> &conn) { OnConnnectCallback(conn); });
  if (use_rpc_flow_pool_) {
    EnableRpcFlowPool();
  }
  LOG_INFO << "HttpServer Listening on [ " << ip << ":" << port << " ]";
}
HttpServer::~HttpServer() = default;

void HttpServer::Start() { loop_->Loop(); }

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
  if (use_rpc_flow_pool_ && rpc_flow_pool_ && request->GetURL() == "/rpc") {
    HandleRpcRequestInPool(conn, *request, close);
    return;  // 有流程池需要自己决定响应报文和时间
  }
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
  server_->OnMessage(std::move(cb));
}

void HttpServer::SetHttpResponseCallBack(std::function<void(const HttpRequest &request, HttpResponse *response)> &&cb) {
  http_call_back_ = std::move(cb);
  server_->OnMessage([this](const std::shared_ptr<Connection> &conn) {
    OnHttpRequest(conn);
  });  // 成员变量函数必须明确对象(无关是否需要本对象元素，类就要明确this)}
}

void HttpServer::HandleRpcRequestInPool(const std::shared_ptr<Connection> &conn, const HttpRequest &request,
                                        bool close) {
  Json req;
  try {
    req = Json::parse(request.GetBody());
  } catch (const Json::parse_error &) {
    Json resp = MakeRespJson(0, false, RpcCode::KRPCBADJSON, "bad json", Json{});
    SendRpcRawResponse(conn, BuildRpcHttpResponse(resp, close, HttpResponse::HttpStatusCode::K400BADREQUEST), close);
    return;
  }

  if (!req.contains("id") || !req["id"].is_number_integer()) {
    Json resp = MakeRespJson(0, false, RpcCode::KRPCBADPARAMS, "bad params", Json{});
    SendRpcRawResponse(conn, BuildRpcHttpResponse(resp, close, HttpResponse::HttpStatusCode::K200K), close);
    return;
  }
  if (!req.contains("method") || !req["method"].is_string() || !req.contains("params") || !req["params"].is_object()) {
    Json resp = MakeRespJson(req.value("id", 0), false, RpcCode::KRPCBADPARAMS, "bad params", Json{});
    SendRpcRawResponse(conn, BuildRpcHttpResponse(resp, close, HttpResponse::HttpStatusCode::K200K), close);
    return;
  }

  int id = req["id"].get<int>();
  std::string method = req["method"].get<std::string>();
  Json params = req["params"];
  auto it = rpc_handlers.find(method);
  if (it == rpc_handlers.end()) {
    Json resp = MakeRespJson(id, false, RpcCode::KRPCMETHODNOTFOUND, "method not found", Json{});
    SendRpcRawResponse(conn, BuildRpcHttpResponse(resp, close, HttpResponse::HttpStatusCode::K200K), close);
    return;
  }

  const uint64_t token = rpc_token_.fetch_add(1, std::memory_order_relaxed); // 内存序
  {
    std::lock_guard<std::mutex> lock(rpc_pending_mtx_);
    rpc_pending_.insert(token);
  }
  std::weak_ptr<Connection> weak_conn = conn;
  conn->GetLoop()->RunAfter(static_cast<double>(kRpcTimeoutMs) / 1000.0,
                            [this, token, id, close, weak_conn]() mutable {
                              bool should_send = false;
                              {
                                std::lock_guard<std::mutex> lock(rpc_pending_mtx_);
                                should_send = rpc_pending_.erase(token) > 0;
                              }
                              if (!should_send) {
                                return;
                              }
                              auto locked = weak_conn.lock(); // 连接关闭了也不发送
                              if (!locked) {
                                return;
                              }
                              Json resp = MakeRespJson(id, false, RpcCode::KRPCTIMEOUT, "timeout", Json{});
                              SendRpcRawResponse(locked,
                                                 BuildRpcHttpResponse(resp, close, HttpResponse::HttpStatusCode::K200K),
                                                 close);
                            });

  RpcHandler handler = it->second;
  rpc_flow_pool_->Add([this, weak_conn, token, id, close, handler, params = std::move(params)]() mutable {
    Json result = Json::object();
    bool ok = false;
    try {
      ok = handler(params, &result);
    } catch (...) {
      ok = false;
    }
    Json resp = ok ? MakeRespJson(id, true, RpcCode::KRPCOK, "OK", result)
                   : MakeRespJson(id, false, RpcCode::KRPCINTERNALERROR, "internal error", Json{});
    std::string wire = BuildRpcHttpResponse(resp, close, HttpResponse::HttpStatusCode::K200K);
    bool should_send = false;
    {
      std::lock_guard<std::mutex> lock(rpc_pending_mtx_);
      should_send = rpc_pending_.erase(token) > 0;
    }
    if (!should_send) {
      return;
    }
    auto locked = weak_conn.lock();
    if (!locked) {
      return;
    }
    EventLoop *loop = locked->GetLoop();
    loop->QueueOneFunc(
        [locked, wire = std::move(wire), close]() mutable { SendRpcRawResponse(locked, std::move(wire), close); });
  });
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

void HttpServer::FileUpload(const HttpRequest *request) {
  size_t b_index = request->GetHeader("Content-Type").find("boundary=");
  std::string boundary = request->GetHeader("Content-Type").substr(b_index + std::string("boundary=").size());

  size_t fn_index = request->GetBody().find("filename");
  if (fn_index == std::string::npos) {
    Errif(true, "Upload filename not found");
    return;
  }
  fn_index += std::string("filename=\"").size();
  size_t fn_end_index = request->GetBody().find("\"\r\n", fn_index);
  std::string filename = request->GetBody().substr(fn_index, fn_end_index - fn_index);

  size_t f_index = request->GetBody().find("\r\n\r\n");
  f_index += std::string("\r\n\r\n").size();
  size_t f_end_index = request->GetBody().find("--" + boundary + "--", f_index);
  std::string file_data = request->GetBody().substr(f_index, f_end_index - f_index);

  // 保存文件
  std::ofstream ofs("../files/" + filename, std::ios::out | std::ios::binary);
  ofs.write(file_data.data(), file_data.size());
  ofs.close();
}
