#pragma once
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include "CoroutineTask.h"
#include "HttpRequest.h"
#include "Logger.h"
#include "Macro.h"
class Connection;
class Server;
class EventLoop;
class HttpRequest;
class HttpResponse;
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
  void Start();
  void OverTime(const std::weak_ptr<Connection> &conn);
  static std::string ReadFileCached(const std::string &filename);

  static void FileUpload(const HttpRequest *request);

 private:
  bool auto_shutdown_ = true;
  bool use_coroutine_ = true;
  std::unique_ptr<Server> server_;
  std::unique_ptr<EventLoop> loop_;
  // std::function<void(const std::shared_ptr<Connection> &conn)> message_call_back_;
  std::function<void(const HttpRequest &request, HttpResponse *response)> http_call_back_;
  coro::DetachedTask HandleHttpSession(std::shared_ptr<Connection> conn);

  DISALLOW_COPY_AND_ASSIGN(HttpServer);
};

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
