#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include "Error.h"
#include "Macro.h"
static const int FIXEDBUFFERSIZE = 4096;
static const int KMAXNUMERICSIZE = 48;

template <int SIZE>
class FixedBuffer {
 public:
  FixedBuffer() : cur_(data_) {}
  ~FixedBuffer() = default;

  void Append(const char *buf, int len) {
    if (GetSpace() > len) {
      memcpy(cur_, buf, len);
      cur_ += len;
    } else {
      Errif(true, "FixedBuffer is full");
    }
  }
  void Add(int len) { cur_ += len; }

  [[nodiscard]] const char *Buffer() const { return data_; }
  [[nodiscard]] const char *GetEnd() const { return data_ + sizeof(data_); }
  [[nodiscard]] char *GetCur() const { return cur_; }
  [[nodiscard]] int GetLength() const { return static_cast<int>(cur_ - data_); }
  [[nodiscard]] int GetSpace() const { return static_cast<int>(GetEnd() - cur_); }

  void Reset() { cur_ = data_; }
  void Clear() {
    memset(data_, '\0', sizeof(data_));
    Reset();
  }

 private:
  char data_[SIZE]{};
  char *cur_ = nullptr;  // 数组已经释放内存，不用显式释放此指针

  DISALLOW_COPY_AND_ASSIGN(FixedBuffer);
};

class LogStream {
 public:
  LogStream();
  ~LogStream() = default;
  LogStream &operator<<(bool v);
  // 整形数据的字符串转换、保存到缓冲区； 内部均调用 void formatInteger(T); 函数
  LogStream &operator<<(int16_t num);
  LogStream &operator<<(uint16_t num);
  LogStream &operator<<(int32_t num);
  LogStream &operator<<(uint32_t num);
  LogStream &operator<<(int64_t num);
  LogStream &operator<<(uint64_t num);

  // 浮点类型数据转换成字符串
  LogStream &operator<<(const float &num);

  LogStream &operator<<(const double &num);
  LogStream &operator<<(char v);
  // 原生字符串输出到缓冲区
  LogStream &operator<<(const char *str);

  // 标准字符串std::string输出到缓冲区
  LogStream &operator<<(const std::string &v);

  void StreamAppend(const char *buf, int len);

  [[nodiscard]] const FixedBuffer<FIXEDBUFFERSIZE> *GetBuffer() const;
  void ClearBuffer();

 private:
  std::unique_ptr<FixedBuffer<FIXEDBUFFERSIZE>> buffer_;
  template <typename T>
  void FormatInteger(T value);

  DISALLOW_COPY_AND_ASSIGN(LogStream);
};

template <typename T>
void LogStream::FormatInteger(T value) {  // 调用对象自己的函数不用传this 减少内存使用
  if (buffer_->GetSpace() > KMAXNUMERICSIZE) {
    char *start = buffer_->GetCur();
    char *end = start;
    do {
      *(end++) = '0' + value % 10;  // ASCII 0~9
      value /= 10;
    } while (value != 0);
    if (value < 0) {
      *(end++) = '-';
    }
    std::reverse(start, end);
    buffer_->Add(static_cast<int>(end - start));
  } else {
    Errif(true, "FixedBuffer is full");
  }
}

class StreamTemplate {  // 定长字符串输出，小心访问越界
 public:
  StreamTemplate(const char *str, int size);
  ~StreamTemplate() = default;  // 不用释放指针，因为是外部元素传入的
  [[nodiscard]] const char *GetStr() const;
  [[nodiscard]] int GetSize() const;

 private:
  const char *str_;
  const int size_;

  DISALLOW_COPY_AND_ASSIGN(StreamTemplate);
};

inline LogStream &operator<<(LogStream &stream, const StreamTemplate &temp) {
  stream.StreamAppend(temp.GetStr(), temp.GetSize());
  return stream;
}

class Formator {  // 用于格式化输出
 public:
  template <typename T>
  Formator(const char *form, T value);
  ~Formator() = default;
  [[nodiscard]] const char *GetValue() const;
  [[nodiscard]] int GetLen() const;

 private:
  char value_[32]{};
  int len_ = 0;

  DISALLOW_COPY_AND_ASSIGN(Formator);
};

template <typename T>
Formator::Formator(const char *form, T value) {
  Errif(!static_cast<bool>(std::is_arithmetic<T>::value), "Value is not arithmetic type");
  len_ = snprintf(value_, sizeof(value_), form, value);  // 转换成字符串
  Errif(len_ > static_cast<int>(sizeof(value_)), "Value is not arithmetic type");
}

inline LogStream &operator<<(LogStream &stream, const Formator &temp) {
  stream.StreamAppend(temp.GetValue(), temp.GetLen());
  return stream;
}
class SourceFile {
 public:
  explicit SourceFile(const char *data);
  const char *data_;
  int size_;
};
inline LogStream &operator<<(LogStream &s, const SourceFile &v) {
  s.StreamAppend(v.data_, v.size_);
  return s;
}
