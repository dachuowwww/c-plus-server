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
  [[nodiscard]] ssize_t GetSize() const;
  [[nodiscard]] const char *GetData() const;
  void Input(const char *data);
  void Clear();
};
