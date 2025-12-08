#pragma once
#include <string>
#include <unordered_map>
#include "Macro.h"
class HttpResponse {
 public:
  enum HttpStatusCode { INKNOWN = 1, BAD_RESQUEST = 400, OK = 200, NOT_FOUND = 404 };
  explicit HttpResponse(bool close);
  ~HttpResponse() = default;
  void SetStatusCode(HttpResponse::HttpStatusCode code);
  void SetStatusMessage(std::string &&message);
  void SetContentType(std::string &&type);
  void SetResponseBody(std::string &&body);
  void AddHeader(std::string &&key, std::string &&value);
  void SetClose();
  [[nodiscard]] bool IfClose() const;
  [[nodiscard]] std::string GetResponse();

 private:
  bool close_;
  HttpStatusCode status_code_;
  std::string status_message_;
  std::string content_type_;
  std::unordered_map<std::string, std::string> header_;
  std::string body_;

  DISALLOW_COPY_AND_ASSIGN(HttpResponse);
};
