#pragma once
#include <functional>
#include <memory>
#include "Macro.h"
class Connection;
class Server;
class EventLoop;
class HttpRequest;
class HttpResponse;
class HttpServer {
 public:
  HttpServer(const char *ip, uint16_t port);
  ~HttpServer();
  void SetMessageCallBack(std::function<void(const std::shared_ptr<Connection> &conn)> &&cb);
  void SetHttpResponseCallBack(std::function<void(const HttpRequest &request, HttpResponse *response)> &&cb);
  void OnHttpRequest(const std::shared_ptr<Connection> &conn);
  void OnHttpReponse(const std::shared_ptr<Connection> &conn);
  void Start();

 private:
  std::unique_ptr<Server> server_;
  std::unique_ptr<EventLoop> loop_;
  std::function<void(const std::shared_ptr<Connection> &conn)> message_call_back_;
  std::function<void(const HttpRequest &request, HttpResponse *response)> http_call_back_;

  DISALLOW_COPY_AND_ASSIGN(HttpServer);
};
