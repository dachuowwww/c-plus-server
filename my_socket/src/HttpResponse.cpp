#include "HttpResponse.h"

HttpResponse::HttpResponse(bool close) : close_(close) { status_code_ = HttpStatusCode::INKNOWN; }

void HttpResponse::SetStatusCode(HttpResponse::HttpStatusCode code) { status_code_ = code; }
void HttpResponse::SetStatusMessage(std::string &&message) { status_message_ = std::move(message); }
void HttpResponse::SetContentType(std::string &&type) { header_["Content-Type"] = type; }
void HttpResponse::SetResponseBody(std::string &&body) { body_ = std::move(body); }
void HttpResponse::AddHeader(std::string &&key, std::string &&value) { header_[std::move(key)] = std::move(value); }
void HttpResponse::SetClose() { close_ = true; }
bool HttpResponse::IfClose() const { return close_; }
std::string HttpResponse::GetResponse() {
  std::string response;
  response += "HTTP/1.1 ";
  response += std::to_string(status_code_);  // 返回数字
  response += " ";
  response += status_message_;
  response += "\r\n";
  if (close_) {
    response += "Connection:close\r\n";
  } else {
    response += "Connection:keep-alive\r\n";
    response += "Content-Length:";
    response += std::to_string(body_.size());
    response += "\r\n";
  }
  for (auto &[key, value] : header_) {
    response += key;
    response += ":";
    response += value;
    response += "\r\n";
  }
  response += "\r\n";
  response += body_;

  return response;
}
