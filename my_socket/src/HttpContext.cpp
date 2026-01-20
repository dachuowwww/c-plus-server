// 解析工具入口
#include "HttpContext.h"
#include "Error.h"
#include "HttpRequest.h"

HttpContext::HttpContext() : request_(std::make_unique<HttpRequest>()) {}

HttpContext::~HttpContext() = default;

bool HttpContext::ParaseRequest(const char *begin, int size) {
  char *start = const_cast<char *>(begin);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
  char *end = start;

  std::string key;
  std::string vers;
  while (state_ != State::COMPLETE && state_ != State::INVAILD && end - start < size) {
    char ch = *end;
    switch (state_) {
      case State::START:
        if (isupper(ch)) {
          state_ = State::METHOD;
        }
        break;
      case State::METHOD:
        if (isblank(ch)) {
          request_->SetMethod(std::string(start, end));
          state_ = State::BEFORE_URI;
          start = end;
        }
        break;
      case State::BEFORE_URI:
        if (isblank(ch)) {
          break;
        }
        if (ch == '/') {
          state_ = State::IN_URI;
          start = end;
        }
        break;
      case State::IN_URI:
        if (isblank(ch)) {
          request_->SetURL(std::string(start, end));
          state_ = State::BEFORE_PROTOCOL;
          start = end;
        } else if (ch == '?') {
          request_->SetURL(std::string(start, end));
          state_ = State::BEFORE_URI_PARAMS_KEY;
          start = end;
        }
        break;
      case State::BEFORE_URI_PARAMS_KEY:
        if (isblank(ch) || ch == '\r' || ch == '\n') {
          state_ = State::INVAILD;
          break;
        } else {
          state_ = State::IN_URI_PARAMS_KEY;
          start = end;
        }
        break;
      case State::IN_URI_PARAMS_KEY:
        if (ch == '=') {
          key = std::string(start, end);
          request_->SetParams(key, "");  // 先存一个空值，等会补上
          state_ = State::BEFORE_URI_PARAMS_VALUE;
        }
        break;
      case State::BEFORE_URI_PARAMS_VALUE:
        if (isblank(ch) || ch == '\r' || ch == '\n') {
          state_ = State::INVAILD;
          break;
        } else {
          state_ = State::IN_URI_PARAMS_VALUE;
          start = end;
        }
        break;
      case State::IN_URI_PARAMS_VALUE:
        if (ch == '&') {
          request_->SetParams(key, std::string(start, end));
          state_ = State::BEFORE_URI_PARAMS_KEY;
          start = end;
        } else if (isblank(ch)) {
          request_->SetParams(key, std::string(start, end));
          state_ = State::BEFORE_PROTOCOL;
          start = end;
        }
        break;
      case State::BEFORE_PROTOCOL:
        if (isblank(ch)) {
          break;
        }
        if (isalpha(ch)) {
          state_ = State::IN_PROTOCOL;
          start = end;
        }
        break;
      case State::IN_PROTOCOL:
        if (ch == '/') {
          request_->SetProtocol(std::string(start, end));
          state_ = State::BEFORE_VERSION;
          start = end;
        }
        break;
      case State::BEFORE_VERSION:
        if (isdigit(ch)) {
          state_ = State::IN_VERSION;
          start = end;
        } else {
          state_ = State::INVAILD;
        }
        break;
      case State::IN_VERSION:
        if (ch == '\r') {
          vers.append(start, end);
          request_->SetVersion(vers);
          state_ = State::CR;
        } else if (ch == '.') {
          vers.append(start, end);
          state_ = State::VERSION_SPLIT;
          start = end;
        }
        break;
      case State::VERSION_SPLIT:
        if (isdigit(ch)) {
          vers.append(".");
          state_ = State::IN_VERSION;
          start = end;
        } else {
          state_ = State::INVAILD;
        }
        break;
      case State::CR:
        if (ch == '\n') {
          state_ = State::CRLF;
          start = end;
        } else {
          state_ = State::INVAILD;
        }
        break;
      case State::CRLF:
        if (isblank(ch)) {
          state_ = State::INVAILD;
        } else if (ch == '\r') {
          state_ = State::CRLFCR;
        } else {
          state_ = State::HEADER_KEY;
          start = end;
        }
        break;
      case State::HEADER_KEY:
        if (ch == ':') {
          key = std::string(start, end);
          request_->SetHeader(key, "");  // 先存一个空值，等会补上
          state_ = State::HEADER_AFTER_COLON;
          start = end;
        } else if (isblank(ch)) {
          key = std::string(start, end);
          request_->SetHeader(key, "");  // 先存一个空值，等会补上
          state_ = State::HEADER_BEFORE_COLON;
          start = end;
        }
        break;
      case State::HEADER_BEFORE_COLON:
        if (isblank(ch)) {
          break;
        } else if (ch == ':') {
          state_ = State::HEADER_AFTER_COLON;
          start = end;
        } else {
          state_ = State::INVAILD;
        }
        break;
      case State::HEADER_AFTER_COLON:
        if (isblank(ch)) {
          break;
        } else {
          state_ = State::HEADER_VALUE;
          start = end;
        }
        break;
      case State::HEADER_VALUE:
        if (ch == '\r') {
          request_->SetHeader(key, std::string(start, end));  // 先存一个空值，等会补上
          state_ = State::CR;
        }
        break;
      case State::CRLFCR:
        if (ch == '\n') {
          if (request_->GetAllHeaders().count("Content-Length")) {
            if (atoi(request_->GetHeader("Content-Length").c_str()) > 0) {
              state_ = State::BODY;
              start = end + 1;
            } else {
              state_ = State::COMPLETE;
            }
          } else {
            if (size > end - begin) {
              state_ = State::BODY;
              start = end + 1;
            } else {
              state_ = State::COMPLETE;
            }
          }
        }
        break;
      case State::BODY: {
        int bodylength = size - static_cast<int>(start - begin);
        request_->SetBody(std::string(
            start, const_cast<char *>(begin) + size));  // NOLINT(cppcoreguidelines-pro-type-const-cast)
        if (bodylength >= atoi(request_->GetHeader("Content-Length").c_str())) {
          state_ = State::COMPLETE;
        }
        break;
      }
      case State::INVAILD:
        Errif(true, "http request parase invaild");
        break;
      default:
        break;
    }
    end++;
  }
  return state_ == State::COMPLETE || state_ == State::BODY;
}

HttpRequest const *HttpContext::GetHttpRequest() const { return this->request_.get(); }

bool HttpContext::IsComplete() const { return state_ == State::COMPLETE; }

void HttpContext::ResetState() { state_ = State::START; }
