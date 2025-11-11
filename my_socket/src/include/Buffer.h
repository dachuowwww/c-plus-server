#pragma once
#include <string>
#include "Macro.h"
class Buffer {
 private:
  std::string buffer_;

 public:
  Buffer() = default;
  ~Buffer() = default;
  void Append(const char *data, ssize_t len);
  [[nodiscard]] ssize_t GetSize() const;
  [[nodiscard]] const char *GetData() const;
  void Clear();
  void SetData(const char *data);
  void SetKeyBoardInput();

  DISALLOW_COPY_AND_ASSIGN(Buffer);
};
