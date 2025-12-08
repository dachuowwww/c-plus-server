#include "HttpServer.h"
#include <iostream>
#include "Connection.h"
#include "EventLoop.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Server.h"

HttpServer::HttpServer(const char *ip, uint16_t port) {
  loop_ = std::make_unique<EventLoop>();
  server_ = std::make_unique<Server>(loop_.get(), ip, port);
  server_->OnMessage([this](const std::shared_ptr<Connection> &conn) {
    OnHttpRequest(conn);
  });  // 成员变量函数必须明确对象(是否需要本对象元素)
}

HttpServer::~HttpServer() = default;

void HttpServer::Start() { loop_->Loop(); }

void HttpServer::OnHttpRequest(const std::shared_ptr<Connection> &conn) {
  HttpContext *context = conn->GetContext();
  if (!context->ParaseRequest(conn->ReadInputBuffer(), conn->ReadInputBufferSize())) {
    conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->SetState(Connection::State::Closed);
    if (conn->IsInEpoll()) {
      conn->RemoveConnection();
    }
  } else {
    std::cout << context->GetHttpRequest()->GetURL()
              << " request successfully. Method:" << context->GetHttpRequest()->GetMethodString() << std::endl;
    OnHttpReponse(conn);
    context->ResetState();
  }
}

void HttpServer::OnHttpReponse(const std::shared_ptr<Connection> &conn) {
  HttpRequest const *request = conn->GetContext()->GetHttpRequest();
  bool close = request->GetHeader("Connection") == "Close" ||
               (request->GetVersionString() == "Http1.0" && request->GetHeader("Connection") == "Keep-Alive");
  auto response = std::make_unique<HttpResponse>(close);
  http_call_back_(*request, response.get());
  // std::cout << response->GetResponse()<< std::endl;
  conn->Send(response->GetResponse().c_str());
  if (response->IfClose()) {
    conn->SetState(Connection::State::Closed);
    conn->RemoveConnection();
  }
}

void HttpServer::SetMessageCallBack(std::function<void(const std::shared_ptr<Connection> &conn)> &&cb) {
  message_call_back_ = std::move(cb);
}

void HttpServer::SetHttpResponseCallBack(std::function<void(const HttpRequest &request, HttpResponse *response)> &&cb) {
  http_call_back_ = std::move(cb);
}
