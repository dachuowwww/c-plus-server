#pragma once
#include <string>
#include <unordered_map>
#include "Macro.h"

class HttpRequest {
 public:
  enum class Method { GET, POST, HEAD, PUT, DELETE, INVAILD };

  enum class Version { HTTP_10, HTTP_11, INVAILD_VERSION };

  HttpRequest() = default;
  ~HttpRequest() = default;

  void SetMethod(const std::string &method);
  [[nodiscard]] std::string GetMethodString() const;

  void SetURL(std::string &&url);
  [[nodiscard]] const std::string &GetURL() const;

  void SetProtocol(std::string &&protocol);
  [[nodiscard]] const std::string &GetProtocol() const;

  void SetVersion(const std::string &version);
  [[nodiscard]] std::string GetVersionString() const;

  void SetParams(const std::string &key, const std::string &value);
  [[nodiscard]] std::string GetParams(const std::string &key) const;
  [[nodiscard]] const std::unordered_map<std::string, std::string> &GetAllParams() const;

  void SetHeader(const std::string &key, const std::string &value);
  [[nodiscard]] std::string GetHeader(const std::string &key) const;
  [[nodiscard]] const std::unordered_map<std::string, std::string> &GetAllHeaders() const;

  void SetBody(std::string &&body);
  [[nodiscard]] const std::string &GetBody() const;

 private:
  Method method_ = Method::INVAILD;
  std::string url_;
  std::string protocol_;
  Version version_ = Version::INVAILD_VERSION;
  std::unordered_map<std::string, std::string> params_;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;

  DISALLOW_COPY_AND_ASSIGN(HttpRequest);
};
