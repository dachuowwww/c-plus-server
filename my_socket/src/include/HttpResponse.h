#pragma once
#include <string>
#include <unordered_map>
#include "Macro.h"
class HttpResponse {
 public:
  enum HttpStatusCode {
    kUnkonwn = 1,
    K100CONTINUE = 100,
    K200K = 200,
    K301K = 301,
    K302K = 302,
    K303K = 303,
    K400BADREQUEST = 400,
    K403FORBIDDEN = 403,
    K404NOTFOUND = 404,
    K500INTERNALSERVERERROR = 500
  };  // 浏览器常用状态码
  explicit HttpResponse(bool close);
  ~HttpResponse() = default;
  void SetStatusCode(HttpResponse::HttpStatusCode code);
  void SetStatusMessage(std::string &&message);
  void SetContentType(std::string &&type);
  void SetResponseBody(std::string &&body);
  void SetContentLength(int length);
  void SetBodyType(std::string &&type);
  void AddHeader(std::string &&key, std::string &&value);
  void SetClose();
  void SetFileId(int fd);
  [[nodiscard]] bool IfClose() const;
  [[nodiscard]] std::string GetPreBody();
  [[nodiscard]] std::string GetResponse();
  [[nodiscard]] const std::string &GetBodyType();
  [[nodiscard]] int GetContentLength();
  [[nodiscard]] int GetFileId();

 private:
  bool close_;
  HttpStatusCode status_code_ = K100CONTINUE;
  std::string status_message_;
  std::string content_type_;
  std::unordered_map<std::string, std::string> header_;
  std::string body_;
  std::string body_type_ = "HTML_TYPE";
  int content_length_ = 0;
  int filefd_ = -1;

  DISALLOW_COPY_AND_ASSIGN(HttpResponse);
};
