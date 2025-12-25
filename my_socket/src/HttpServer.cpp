#include "HttpServer.h"
#include <arpa/inet.h>
#include <iostream>
#include "Connection.h"
#include "EventLoop.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Logger.h"
#include "Server.h"
#include "TimeStamp.h"

HttpServer::HttpServer(const char *ip, uint16_t port) {
  loop_ = std::make_unique<EventLoop>();
  server_ = std::make_unique<Server>(loop_.get(), ip, port);
  server_->OnConnect([this](const std::shared_ptr<Connection> &conn) { OnConnnectCallback(conn); });
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
    loop_->RunAfter(10.0, bind(&HttpServer::OverTime, this, std::weak_ptr<Connection>(conn)));
  }
}

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
              << ", request successfully. Method:" << context->GetHttpRequest()->GetMethodString() << std::endl;
    OnHttpReponse(conn);
    context->ResetState();
    if (auto_shutdown_) {
      conn->UpdateTimeStamp();
    }
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
  server_->OnMessage(std::move(message_call_back_));
}

void HttpServer::SetHttpResponseCallBack(std::function<void(const HttpRequest &request, HttpResponse *response)> &&cb) {
  http_call_back_ = std::move(cb);
  server_->OnMessage([this](const std::shared_ptr<Connection> &conn) {
    OnHttpRequest(conn);
  });  // 成员变量函数必须明确对象(无关是否需要本对象元素，类就要明确this)
}

void HttpServer::OnTimerEvery(double interval, std::function<void()> &&cb) { loop_->RunEvery(interval, std::move(cb)); }

void HttpServer::OverTime(const std::weak_ptr<Connection> &conn) {
  auto conn_ptr = conn.lock();
  if (conn_ptr) {
    if (TimeStamp::AddTime(conn_ptr->GetTimeStamp(), AUTOCLOSETIMEOUT) < TimeStamp::Now()) {
      conn_ptr->RemoveConnection();
    } else {
      loop_->RunAfter((double)(AUTOCLOSETIMEOUT - (TimeStamp::Now().Time()-conn_ptr->GetTimeStamp().Time()) / MICROSECOND_2_SECOND) + 1.0,
                      bind(&HttpServer::OverTime, this, conn));
    }
  }
}
