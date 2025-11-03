#pragma once
#include <string>

class Buffer {
 private:
  std::string buffer_;

 public:
  Buffer();
  ~Buffer();
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
  Buffer(Buffer &&) = delete;
  Buffer &operator=(Buffer &&) = delete;
  void Append(const char *, const ssize_t);
  ssize_t GetSize();
  const char *ReadAll();
  void Clear();
  void GetLine();
};
