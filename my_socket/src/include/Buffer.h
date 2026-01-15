#pragma once
#include <string>
#include <vector>
#include "Macro.h"

static const int KPREPENDINDEX = 8;   // prependindex长度
static const int KINITALSIZE = 1024;  // 初始化开辟空间长度
class Buffer {
 public:
  Buffer();
  ~Buffer() = default;
  void Append(const char *data);
  void Append(const char *data, ssize_t len); // 一般情况最好声明长度，避免多次调用strlen
  void Append(const std::string &data);
  [[nodiscard]] int ReadableBytes() const;
  [[nodiscard]] int WritableBytes() const;
  [[nodiscard]] int PrepenableBytes() const;
  void EnsureWritableBytes(int len);

  [[nodiscard]] char *Begin();
  [[nodiscard]] const char *Begin() const;

  [[nodiscard]] char *BeginRead();
  [[nodiscard]] const char *BeginRead() const;

  [[nodiscard]] char *BeginWrite();
  [[nodiscard]] const char *BeginWrite() const;

  [[nodiscard]] char *Peek();
  [[nodiscard]] const char *Peek() const;
  [[nodiscard]] std::string PeekAsString(int len) const;
  [[nodiscard]] std::string PeekAllString() const;

  void Retreive(int len);
  [[nodiscard]] std::string RetreiveAsString(int len);
  void RetreiveAll();
  [[nodiscard]] std::string RetreiveAllAsString();

  void RetrieveUtil(const char *end);
  [[nodiscard]] std::string RetrieveUtilAsString(const char *end);

  void AppendKeyBoard();

 private:
  std::vector<char> buffer_;
  int read_index_;
  int write_index_;

  DISALLOW_COPY_AND_ASSIGN(Buffer);
};
