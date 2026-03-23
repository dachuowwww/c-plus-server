#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include "HttpRequest.h"

class Connection;
class Server;
class EventLoop;
class HttpRequest;
class HttpResponse;
class ThreadPool;
const double AUTOCLOSETIMEOUT = 10.0;

class HttpServer {
 public:
  HttpServer(const char *ip, uint16_t port);
  ~HttpServer();
  void SetMessageCallBack(std::function<void(const std::shared_ptr<Connection> &conn)> &&cb);
  void SetHttpResponseCallBack(std::function<void(const HttpRequest &request, HttpResponse *response)> &&cb);
  void OnConnnectCallback(const std::shared_ptr<Connection> &conn);
  void OnHttpRequest(const std::shared_ptr<Connection> &conn);
  void OnHttpReponse(const std::shared_ptr<Connection> &conn);
  void OnTimerEvery(double interval, std::function<void()> &&cb);
  void EnableRpcFlowPool(unsigned int size = std::thread::hardware_concurrency());
  void DisableRpcFlowPool();
  void Start();
  void OverTime(const std::weak_ptr<Connection> &conn);

  static void FileUpload(const HttpRequest *request);

  static std::string ReadFile(const std::string &filename);

  // static void HandleRpcRequest(const HttpRequest &request, HttpResponse *response);

 private:
  bool auto_shutdown_ = true;
  bool use_rpc_flow_pool_ = true;
  std::atomic<uint64_t> rpc_token_{1};
  std::unordered_set<uint64_t> rpc_pending_;
  std::mutex rpc_pending_mtx_;
  std::unique_ptr<Server> server_;
  std::unique_ptr<EventLoop> loop_;
  std::unique_ptr<ThreadPool> rpc_flow_pool_;
  // std::function<void(const std::shared_ptr<Connection> &conn)> message_call_back_;
  std::function<void(const HttpRequest &request, HttpResponse *response)> http_call_back_;
  void HandleRpcRequestInPool(const std::shared_ptr<Connection> &conn, const HttpRequest &request, bool close);
  DISALLOW_COPY_AND_ASSIGN(HttpServer);
};
