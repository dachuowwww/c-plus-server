#pragma once
#include <string>

class Buffer {
 private:
  std::string buffer_;

 public:
  Buffer() = default;
  ~Buffer() = default;
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
  Buffer(Buffer &&) = delete;
  Buffer &operator=(Buffer &&) = delete;
  void Append(const char *data, ssize_t len);
  ssize_t GetSize();
  const char *ReadAll();
  void Clear();
  void GetKeyboardLine();
  void GetData(const char *data);
};
