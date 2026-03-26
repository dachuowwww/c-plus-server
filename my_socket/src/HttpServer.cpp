#include "HttpServer.h"
#include <arpa/inet.h>
#include <algorithm>
#include <cctype>
#include <deque>
#include <fstream>
#include <iostream>
#include <unordered_map>
// #include <nlohmann/json.hpp>  // JSON版本暂时注释
#include "Connection.h"
#include "Error.h"
#include "EventLoop.h"
#include "HttpContext.h"
#include "HttpResponse.h"
#include "Logger.h"
#include "Macro.h"
// #include "Metrics.h"
#include "RpcEchoSerializers.h"
#include "RpcEchoTypes.h"
#include "RpcJsonSerializer.h"
#include "RpcProto.h"
#include "RpcSerialization.h"
#include "Server.h"
#include "ThreadPool.h"
#include "TimeStamp.h"
#include "rpc.pb.h"

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
using RpcCode = HttpResponse::RpcCode;
constexpr int kRpcTimeoutMs = 2000;

class RpcMethodHandler {
 public:
  virtual ~RpcMethodHandler() = default;
  virtual bool Handle(const std::string &serializer_type, const std::string &request_bytes, std::string *response_bytes,
                      std::string *error_message) = 0;
};

template <typename RequestType, typename ResponseType> // 多态分发，输入输出类型都由模板参数指定
class TypedRpcMethodHandler : public RpcMethodHandler {  // 方法封装，输入序列化器类型，解码请求内容，输出编码后响应内容
 public:
  using HandlerFunc = std::function<ResponseType(const RequestType &)>;
  explicit TypedRpcMethodHandler(HandlerFunc handler) : handler_(std::move(handler)) {}

  bool Handle(const std::string &serializer_type, const std::string &request_bytes, std::string *response_bytes,
              std::string *error_message) override {
    RequestType req{};
    auto *request_type_serializer = SerializerRegistry::Instance().GetTypeSerializer<RequestType>(
        serializer_type);  // 优先在注册表查找使用类型特定的序列化器
    if (request_type_serializer) {
      if (!request_type_serializer->Deserialize(request_bytes, &req)) {
        if (error_message) {
          *error_message = "deserialize request failed";
        }
        return false;
      }
    } else {
      auto *serializer = SerializerRegistry::Instance().GetSerializer(serializer_type);
      if (!serializer) {
        if (error_message) {
          *error_message = "unknown serializer";
        }
        return false;
      }
      const auto *desc = ReflectionRegistry::Instance().GetDescriptor(
          RpcTypeRegistry::Instance()
              .GetTypeName<RequestType>());  // 通过类型注册表找到类型名，再通过反射注册表找到描述符
      if (!desc) {
        if (error_message) {
          *error_message = "request reflection not registered";
        }
        return false;
      }
      if (!serializer->Deserialize(request_bytes, &req, *desc)) {
        if (error_message) {
          *error_message = "deserialize request failed";
        }
        return false;
      }
    }
    // 解码消息体成通用对象完毕
    ResponseType resp_obj = handler_(req);

    auto *response_type_serializer = SerializerRegistry::Instance().GetTypeSerializer<ResponseType>(serializer_type);
    if (response_type_serializer) {
      if (!response_type_serializer->Serialize(resp_obj, response_bytes)) {
        if (error_message) {
          *error_message = "serialize response failed";
        }
        return false;
      }
      return true;
    }

    auto *serializer = SerializerRegistry::Instance().GetSerializer(serializer_type);
    if (!serializer) {
      if (error_message) {
        *error_message = "unknown serializer";
      }
      return false;
    }
    const auto *desc =
        ReflectionRegistry::Instance().GetDescriptor(RpcTypeRegistry::Instance().GetTypeName<ResponseType>());
    if (!desc) {
      if (error_message) {
        *error_message = "response reflection not registered";
      }
      return false;
    }
    if (!serializer->Serialize(&resp_obj, *desc, response_bytes)) {
      if (error_message) {
        *error_message = "serialize response failed";
      }
      return false;
    }
    return true;
  }

 private:
  HandlerFunc handler_;
};

my_socket_rpc::RpcResponse MakeRespProto(int id, bool ok, RpcCode code, const std::string &message,
                                         const std::string &params, const std::string &serializer_type) {
  my_socket_rpc::RpcResponse resp;
  resp.set_id(id);
  resp.set_ok(ok);
  resp.set_code(static_cast<int>(code));
  resp.set_message(message);
  resp.set_serializer_type(serializer_type);
  if (ok) {
    resp.set_params(params);
  }
  return resp;
}

std::string BuildRpcHttpResponse(const std::string &body, bool close, HttpResponse::HttpStatusCode status) {
  HttpResponse response(close);
  response.SetStatusCode(status);
  response.SetStatusMessage(status == HttpResponse::HttpStatusCode::K400BADREQUEST ? "BAD_RESQUEST" : "OK");
  // response.SetContentType("application/json; charset=UTF-8");
  // response.SetResponseBody(resp.dump());
  response.SetContentType("application/x-protobuf");
  response.SetResponseBody(std::string(body));
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
// const std::unordered_map<std::string, RpcHandler> rpc_handlers = {{"Echo", [](const Json &params, Json *result) {
//                                                                      *result = params;
//                                                                      return true;
//                                                                    }}};
// const std::unordered_map<std::string, RpcHandler> rpc_handlers = {
//     {"Echo",
//      [](const std::string &in_params, std::string *out_results) {
//        my_socket_rpc::EchoRequest req;
//        if (!rpcproto::DecodeEchoRequest(in_params, &req)) {
//          return false;
//        }
//        my_socket_rpc::EchoResponse resp;
//        resp.set_msg(req.msg()); // 只取protobuf中的信息部分
//        return rpcproto::EncodeEchoResponse(resp, out_results);
//      }},
// };
std::unordered_map<std::string, std::unique_ptr<RpcMethodHandler>> g_rpc_methods;
std::once_flag g_rpc_init_flag;

void RegisterRpcPlug() {
  std::call_once(g_rpc_init_flag, []() {
    REGISTER_RPC_SERIALIZER("json", RpcJsonSerializer);
    REGISTER_RPC_TYPE(rpc_model::EchoRequest, "EchoRequest");
    REGISTER_RPC_TYPE(rpc_model::EchoResponse, "EchoResponse");
    // 注册反射信息，通过类型找到相应描述符和类型
    rpc_model::EchoRequest::RegisterReflection();
    rpc_model::EchoResponse::RegisterReflection();

    // 仅注册 protobuf 的类型特定序列化器，json 走通用反射序列化器。
    REGISTER_RPC_TYPE_SERIALIZER(rpc_model::EchoRequest, "protobuf", EchoRequestProtobufSerializer);
    REGISTER_RPC_TYPE_SERIALIZER(rpc_model::EchoResponse, "protobuf", EchoResponseProtobufSerializer);

    // Legacy: 早期直接类型 json 序列化器注册方式
    // REGISTER_RPC_TYPE_SERIALIZER(rpc_model::EchoRequest, "json", EchoRequestJsonSerializer);
    // REGISTER_RPC_TYPE_SERIALIZER(rpc_model::EchoResponse, "json", EchoResponseJsonSerializer);

    g_rpc_methods["Echo"] = std::make_unique<TypedRpcMethodHandler<rpc_model::EchoRequest, rpc_model::EchoResponse>>(
        [](const rpc_model::EchoRequest &request) {
          rpc_model::EchoResponse response;
          response.msg = request.msg;
          return response;
        });

    // Legacy: 早期写死 map 方式
    // using RpcHandler = std::function<bool(const std::string &payload, std::string *out_payload)>;
    // const std::unordered_map<std::string, RpcHandler> rpc_handlers = {
    //     {"Echo",
    //      [](const std::string &in_params, std::string *out_results) {
    //        my_socket_rpc::EchoRequest req;
    //        if (!rpcproto::DecodeEchoRequest(in_params, &req)) {
    //          return false;
    //        }
    //        my_socket_rpc::EchoResponse resp;
    //        resp.set_msg(req.msg());
    //        return rpcproto::EncodeEchoResponse(resp, out_results);
    //      }},
    // };
  });
}
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

std::string HttpServer::ReadFile(const std::string &filename) {
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
  return content;
}
//////////////////////////////// 文件缓存池
// namespace {
// struct CachedFile {
//   std::string data;
//   size_t size = 0;
// };

// constexpr size_t kFileCacheMaxEntries = 128;             // 最大缓存文件数量
// constexpr size_t kFileCacheMaxBytes = 32 * 1024 * 1024;  // 最大缓存总字节数（32MB）

// std::mutex g_file_cache_mutex;
// std::unordered_map<std::string, CachedFile> g_file_cache;
// std::deque<std::string> g_file_cache_order;  // 用于记录缓存文件的访问顺序，便于实现FIFO淘汰策略
// size_t g_file_cache_bytes = 0;               // 记录当前缓存的总字节数，便于判断是否超过限制

// void EvictCacheIfNeeded(size_t incoming_size) {
//   while (!g_file_cache_order.empty() &&
//          (g_file_cache.size() >= kFileCacheMaxEntries || g_file_cache_bytes + incoming_size > kFileCacheMaxBytes)) {
//     const std::string &key = g_file_cache_order.front();
//     g_file_cache_order.pop_front();
//     auto it = g_file_cache.find(key);
//     if (it != g_file_cache.end()) {
//       g_file_cache_bytes -= it->second.size;
//       g_file_cache.erase(it);
//     }
//   }
// }
// }  // namespace

// std::string HttpServer::ReadFileCached(const std::string &filename) {  // 简单缓存/文件内容池
//   {
//     std::lock_guard<std::mutex> lock(g_file_cache_mutex);
//     auto it = g_file_cache.find(filename);
//     if (it != g_file_cache.end()) {
//       return it->second.data;
//     }
//   }

//   std::ifstream is(filename.c_str(), std::ifstream::in | std::ifstream::binary);
//   if (!is.is_open()) {
//     Errif(true, "ReadFile: open file failed!");
//     return "";
//   }
//   is.seekg(0, std::ifstream::end);
//   std::streamoff length = is.tellg();
//   if (length <= 0) {
//     return "";
//   }
//   is.seekg(0, std::ifstream::beg);
//   std::string content;
//   content.resize(static_cast<size_t>(length));
//   is.read(content.data(), length);
//   is.close();
//   {
//     std::lock_guard<std::mutex> lock(g_file_cache_mutex);
//     auto it = g_file_cache.find(filename);
//     if (it != g_file_cache.end()) {
//       return it->second.data;
//     }
//     const size_t size = content.size();
//     if (size <= kFileCacheMaxBytes) {
//       EvictCacheIfNeeded(size);
//       g_file_cache_order.push_back(filename);
//       g_file_cache_bytes += size;
//       g_file_cache.emplace(filename, CachedFile{content, size});
//     }
//   }

//   return content;
// }
/////////////////////////////////
HttpServer::HttpServer(const char *ip, uint16_t port) {
  RegisterRpcPlug();
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
  // Metrics::OnRequest();

  if (!context->ParaseRequest(conn->RetriveInputBuffer().c_str(), size)) {
    // Metrics::OnParseError();
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
  // my_socket_rpc::RpcRequest req;
  // if (!rpcproto::DecodeRpcRequest(request.GetBody(), &req)) { ... }
  // const std::string method = req.method();
  // const std::string in_params = req.params();
  // auto it = rpc_handlers.find(method);
  // RpcHandler handler = it->second;
  // rpc_flow_pool_->Add([handler, in_params]() { handler(in_params, &out_results); });
  // 解析protobuf信息包，构建响应，流程池处理，超时控制，线程安全的待处理RPC跟踪，以及最终响应发送回客户端的完整流程。
  my_socket_rpc::RpcRequest req;
  if (!rpcproto::DecodeRpcRequest(request.GetBody(), &req)) {
    auto resp = MakeRespProto(0, false, RpcCode::KRPCBADJSON, "bad protobuf", "", "protobuf");
    std::string wire;
    if (rpcproto::EncodeRpcResponse(resp, &wire)) {
      SendRpcRawResponse(conn, BuildRpcHttpResponse(wire, close, HttpResponse::HttpStatusCode::K400BADREQUEST), close);
    }
    return;
  }

  if (req.method().empty()) {
    const std::string serializer_type = req.serializer_type().empty() ? "json" : req.serializer_type();
    auto resp = MakeRespProto(req.id(), false, RpcCode::KRPCBADPARAMS, "bad params", "", serializer_type);
    std::string wire;
    if (rpcproto::EncodeRpcResponse(resp, &wire)) {
      SendRpcRawResponse(conn, BuildRpcHttpResponse(wire, close, HttpResponse::HttpStatusCode::K200K), close);
    }
    return;
  }

  const int id = req.id();
  const std::string method = req.method();
  const std::string in_params = req.params();
  const std::string serializer_type = req.serializer_type().empty() ? "json" : req.serializer_type();
  auto it = g_rpc_methods.find(method);
  if (it == g_rpc_methods.end()) {
    auto resp = MakeRespProto(id, false, RpcCode::KRPCMETHODNOTFOUND, "method not found", "", serializer_type);
    std::string wire;
    if (rpcproto::EncodeRpcResponse(resp, &wire)) {
      SendRpcRawResponse(conn, BuildRpcHttpResponse(wire, close, HttpResponse::HttpStatusCode::K200K), close);
    }
    return;
  }

  const uint64_t token = rpc_token_.fetch_add(1, std::memory_order_relaxed);
  {
    std::lock_guard<std::mutex> lock(rpc_pending_mtx_);
    rpc_pending_.insert(token);
  }
  std::weak_ptr<Connection> weak_conn =
      conn;  // 捕获连接的弱指针，避免妨碍连接的正常生命周期管理，同时在异步操作中安全地访问连接对象。
  conn->GetLoop()->RunAfter(
      static_cast<double>(kRpcTimeoutMs) / 1000.0, [this, token, id, close, weak_conn, serializer_type]() mutable {
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
        auto resp = MakeRespProto(id, false, RpcCode::KRPCTIMEOUT, "timeout", "", serializer_type);
        std::string wire;
        if (!rpcproto::EncodeRpcResponse(resp, &wire)) {
          return;
        }
        SendRpcRawResponse(locked, BuildRpcHttpResponse(wire, close, HttpResponse::HttpStatusCode::K200K), close);
      });

  RpcMethodHandler *method_handler = it->second.get();
  rpc_flow_pool_->Add([this, weak_conn, token, id, close, method_handler, serializer_type, in_params]() mutable {
    std::string out_results;
    std::string error_message;
    bool ok = false;
    try {
      ok = method_handler->Handle(serializer_type, in_params, &out_results, &error_message);
    } catch (...) {
      ok = false;
      error_message = "internal error";
    }
    if (error_message.empty()) {
      error_message = ok ? "OK" : "internal error";
    }
    RpcCode fail_code = RpcCode::KRPCINTERNALERROR;
    if (!ok && (error_message.find("deserialize") != std::string::npos ||
                error_message.find("serializer") != std::string::npos ||
                error_message.find("reflection") != std::string::npos)) {
      fail_code = RpcCode::KRPCBADPARAMS;
    }
    auto resp = ok ? MakeRespProto(id, true, RpcCode::KRPCOK, error_message, out_results, serializer_type)
                   : MakeRespProto(id, false, fail_code, error_message, "", serializer_type);
    std::string resp_bytes;
    if (!rpcproto::EncodeRpcResponse(resp, &resp_bytes)) {
      return;
    }
    std::string wire = BuildRpcHttpResponse(resp_bytes, close, HttpResponse::HttpStatusCode::K200K);
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
