#include "HttpResponse.h"

HttpResponse::HttpResponse(bool close) : close_(close) {}

void HttpResponse::SetStatusCode(HttpResponse::HttpStatusCode code) { status_code_ = code; }
void HttpResponse::SetStatusMessage(std::string &&message) { status_message_ = std::move(message); }
void HttpResponse::SetContentType(std::string &&type) { header_["Content-Type"] = type; }
void HttpResponse::SetContentLength(int length) { content_length_ = length; }
void HttpResponse::SetBodyType(std::string &&type) { body_type_ = std::move(type); }
void HttpResponse::SetResponseBody(std::string &&body) { body_ = std::move(body); }
void HttpResponse::AddHeader(std::string &&key, std::string &&value) { header_[std::move(key)] = std::move(value); }
void HttpResponse::SetClose() { close_ = true; }
bool HttpResponse::IfClose() const { return close_; }
void HttpResponse::SetFileId(int fd) { filefd_ = fd; }
int HttpResponse::GetFileId() { return filefd_; }
std::string HttpResponse::GetPreBody() {
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
    response += "Content-Length: ";
    response += std::to_string(body_.size());
    response += "\r\n";
  }
  for (auto &[key, value] : header_) {
    response += key;
    response += ": ";
    response += value;
    response += "\r\n";
  }
  response += "Cache-Control: no-store, no-cache, must-revalidate\r\n";
  response += "\r\n";
  return response;
}
std::string HttpResponse::GetResponse() { return GetPreBody() += body_; }
const std::string &HttpResponse::GetBodyType() { return body_type_; }
int HttpResponse::GetContentLength() { return content_length_; }
