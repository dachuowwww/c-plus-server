#include "HttpServer.h"
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <memory>
#include "AsyncLogging.h"
#include "Connection.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Logger.h"
#include "TimeStamp.h"

std::string ReadFile(const std::string &filename) {
  std::ifstream is(filename.c_str(), std::ifstream::in);
  if (!is.is_open()) {
    LOG_ERROR << "ReadFile: open file " << filename << " failed!";
    return "";
  }
  is.seekg(0, std::ifstream::end);
  int length = static_cast<int>(is.tellg());
  is.seekg(0, std::ifstream::beg);
  char *buffer = new char[length];
  is.read(buffer, length);
  is.close();
  std::string content(buffer, length);
  delete[] buffer;
  return content;
}

void Message(const std::shared_ptr<Connection> &conn) {  // 注册回调函数,需要修改内部元素所以不能设为const
  // conn->Read();
  LOG_INFO << "New message from client " << conn->GetFd() << " : " << conn->ReadInputBuffer();
  conn->Send(conn->RetriveInputBuffer());
}

void Http(const HttpRequest &request, HttpResponse *response) {
  if (request.GetMethodString() == "GET") {
    const std::string& url = request.GetURL();
    if (url == "/select") {
      response->SetContentType("text/html; charset=UTF-8");
      response->SetResponseBody(ReadFile("../static/select.html"));
      response->SetStatusCode(HttpResponse::HttpStatusCode::OK);
      response->SetStatusMessage("OK");
    } else if (url == "/hello") {
      response->SetContentType("text/html; charset=UTF-8");
      response->SetResponseBody("<font color=\"red\">您好!</font>");
    } else if (url == "/helloyyq") {
      response->SetContentType("text/plain; charset=UTF-8");
      response->SetResponseBody("Hello 谢可欣宝宝!\n");
      response->SetStatusCode(HttpResponse::HttpStatusCode::OK);
      response->SetStatusMessage("OK");
    } else if (url == "/") {
      response->SetContentType("text/html; charset=UTF-8");
      response->SetResponseBody(ReadFile("../static/index.html"));
      response->SetStatusCode(HttpResponse::HttpStatusCode::OK);
      response->SetStatusMessage("OK");
    } else if (url == "/xkx.jpg") {
      response->SetContentType("image/jpeg");
      response->SetResponseBody(ReadFile("../static/xkx.jpg"));
      response->SetStatusCode(HttpResponse::HttpStatusCode::OK);
      response->SetStatusMessage("OK");
    } else if (url == "/mhw") {
      response->SetContentType("text/html; charset=UTF-8");
      response->SetResponseBody(ReadFile("../static/mhw.html"));
      response->SetStatusCode(HttpResponse::HttpStatusCode::OK);
      response->SetStatusMessage("OK");
    } else if (url == "/cat.jpg") {
      response->SetContentType("image/jpeg");
      response->SetResponseBody(ReadFile("../static/cat.jpg"));
      response->SetStatusCode(HttpResponse::HttpStatusCode::OK);
      response->SetStatusMessage("OK");
    } else {
      response->SetContentType("text/plain");
      response->SetResponseBody("Sorry,not found.\n");
      response->SetStatusCode(HttpResponse::HttpStatusCode::NOT_FOUND);
      response->SetStatusMessage("NOT_FOUND");
      response->SetClose();
    }
  } else if (request.GetMethodString() == "POST") {
    if (request.GetURL() == "/login") {
      const std::string& body = request.GetBody();
      int user_pos = body.find("username=");
      int pass_pos = body.find("password=");

      user_pos += 9;
      pass_pos += 9;

      int and_pos = body.find('&', user_pos);
      int end_pos = body.length();
      std::string username = body.substr(user_pos, and_pos - user_pos);
      std::string password = body.substr(pass_pos, end_pos - pass_pos);
      if (username == "xkx" && password == "pig") {
        response->SetResponseBody("Login successfully!\n");
      } else {
        response->SetResponseBody("Login failed!\n");
      }
      response->SetContentType("text/plain; charset=UTF-8");
      response->SetStatusCode(HttpResponse::HttpStatusCode::OK);
      response->SetStatusMessage("OK");
    }
  } else {
    response->SetStatusCode(HttpResponse::HttpStatusCode::BAD_RESQUEST);
    response->SetStatusMessage("BAD_RESQUEST");
    response->SetClose();
  }
}

void Every() { std::cout << TimeStamp::Now().ToFormattedString() << std::endl; }
std::unique_ptr<AsyncLogging> async_log;
void AsyncOutput(const char *msg, int len) { async_log->Append(msg, len); }
void AsyncFlush() { async_log->Flush(); }
int main(int argc, char *argv[]) {
  std::string ip = "127.0.0.1";
  int port = 8888;
  if (argc == 3) {
    ip = argv[1];
    port = std::atoi(argv[2]);
  }
  async_log = std::make_unique<AsyncLogging>("");
  Logger::SetOutput(AsyncOutput);
  Logger::SetFlush(AsyncFlush);
  async_log->Start();
  auto httpserver = std::make_unique<HttpServer>(ip.c_str(), port);
  if (argc == 2) {  // 加一个参数变echo_server
    httpserver->SetMessageCallBack(Message);
  } else {
    httpserver->SetHttpResponseCallBack(Http);
  }
  // httpserver->OnTimerEvery(3.0, Every);

  httpserver->Start();
  return 0;
}
