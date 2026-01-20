#pragma once
#include <memory>
#include "Macro.h"
class HttpRequest;
class HttpContext {
 public:
  enum class State {
    INVAILD,
    START,
    METHOD,
    BEFORE_URI,
    IN_URI,
    BEFORE_URI_PARAMS_KEY,
    IN_URI_PARAMS_KEY,
    BEFORE_URI_PARAMS_VALUE,
    IN_URI_PARAMS_VALUE,

    BEFORE_PROTOCOL,
    IN_PROTOCOL,

    BEFORE_VERSION,
    IN_VERSION,
    VERSION_SPLIT,

    HEADER_KEY,
    HEADER_BEFORE_COLON,
    HEADER_AFTER_COLON,
    HEADER_VALUE,

    CR,
    LF,
    CRLF,
    CRLFCR,

    BODY,  // content-length

    COMPLETE
  };

  HttpContext();
  ~HttpContext();

  bool ParaseRequest(const char *begin, int size);
  void ResetState();
  [[nodiscard]] HttpRequest const *GetHttpRequest() const;
  [[nodiscard]] bool IsComplete() const;

 private:
  std::unique_ptr<HttpRequest> request_;
  State state_ = State::START;

  DISALLOW_COPY_AND_ASSIGN(HttpContext);
};
