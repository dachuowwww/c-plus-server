// 解析内容
#include "HttpRequest.h"

void HttpRequest::SetMethod(const std::string &method) {
  if (method == "GET") {
    method_ = Method::GET;
  } else if (method == "POST") {
    method_ = Method::POST;
  } else if (method == "HEAD") {
    method_ = Method::HEAD;
  } else if (method == "PUT") {
    method_ = Method::PUT;
  } else if (method == "DELETE") {
    method_ = Method::DELETE;
  } else {
    method_ = Method::INVAILD;
  }
}
std::string HttpRequest::GetMethodString() const {
  std::string method;
  if (method_ == Method::GET) {
    method = "GET";
  } else if (method_ == Method::POST) {
    method = "POST";
  } else if (method_ == Method::HEAD) {
    method = "HEAD";
  } else if (method_ == Method::PUT) {
    method = "PUT";
  } else if (method_ == Method::DELETE) {
    method = "DELETE";
  } else {
    method = "INVAILD";
  }
  return method;
}

void HttpRequest::SetURL(std::string &&url) { url_ = std::move(url); }
const std::string &HttpRequest::GetURL() const { return url_; }

void HttpRequest::SetProtocol(std::string &&protocol) { protocol_ = std::move(protocol); }
const std::string &HttpRequest::GetProtocol() const { return protocol_; }

void HttpRequest::SetVersion(const std::string &version) {
  if (version == "1.0") {
    version_ = Version::HTTP_10;
  }
  if (version == "1.1") {
    version_ = Version::HTTP_11;
  } else {
    version_ = Version::INVAILD_VERSION;
  }
}
std::string HttpRequest::GetVersionString() const {
  std::string version;
  if (version_ == Version::HTTP_10) {
    version = "Http1.0";
  }
  if (version_ == Version::HTTP_11) {
    version = "Http1.1";
  } else {
    version = "Invaild Version";
  }
  return version;
}

void HttpRequest::SetParams(const std::string &key, const std::string &value) { params_[key] = value; }

std::string HttpRequest::GetParams(const std::string &key) const {
  auto it = params_.find(key);
  if (it != params_.end()) {
    return it->second;
  }
  return "";
}
const std::unordered_map<std::string, std::string> &HttpRequest::GetAllParams() const { return params_; }

void HttpRequest::SetHeader(const std::string &key, const std::string &value) { headers_[key] = value; }
std::string HttpRequest::GetHeader(const std::string &key) const {
  auto it = headers_.find(key);
  if (it != headers_.end()) {
    return it->second;
  }
  return "";
}
const std::unordered_map<std::string, std::string> &HttpRequest::GetAllHeaders() const { return headers_; }
void HttpRequest::SetBody(std::string &&body) { body_ = std::move(body); }

const std::string &HttpRequest::GetBody() const { return body_; }
