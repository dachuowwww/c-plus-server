#include "LogStream.h"
LogStream::LogStream() { buffer_ = std::make_unique<FixedBuffer<FIXEDBUFFERSIZE>>(); }
LogStream &LogStream::operator<<(bool v) {
  buffer_->Append(v ? "1" : "0", 1);
  return *this;
}
// 整形数据的字符串转换、保存到缓冲区； 内部均调用 void formatInteger(T);
LogStream &LogStream::operator<<(int16_t num) { return (*this) << static_cast<int>(num); }
LogStream &LogStream::operator<<(uint16_t num) { return (*this) << static_cast<unsigned int>(num); }
LogStream &LogStream::operator<<(int32_t num) {
  FormatInteger(num);
  return *this;
}
LogStream &LogStream::operator<<(uint32_t num) {
  FormatInteger(num);
  return *this;
}
LogStream &LogStream::operator<<(int64_t num) {
  FormatInteger(num);
  return *this;
}
LogStream &LogStream::operator<<(uint64_t num) {
  FormatInteger(num);
  return *this;
}

// 浮点类型数据转换成字符串
LogStream &LogStream::operator<<(const float &num) { return (*this) << static_cast<double>(num); }

LogStream &LogStream::operator<<(const double &num) {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%g", num);  // 转换成字符串
  buffer_->Append(buf, len);
  return *this;
}

LogStream &LogStream::operator<<(char v) {
  buffer_->Append(&v, 1);
  return *this;
}

// 原生字符串输出到缓冲区 结尾带'\0'的字符串
LogStream &LogStream::operator<<(const char *str) {
  if (str) {
    buffer_->Append(str, strlen(str));
  } else {
    buffer_->Append("(null)", 6);
  }

  return *this;
}

// 标准字符串std::string输出到缓冲区
LogStream &LogStream::operator<<(const std::string &v) {
  buffer_->Append(v.c_str(), v.size());
  return *this;
}

void LogStream::StreamAppend(const char *buf, int len) { buffer_->Append(buf, len); }

const FixedBuffer<FIXEDBUFFERSIZE> *LogStream::GetBuffer() const { return buffer_.get(); }
void LogStream::ClearBuffer() { buffer_->Clear(); }

StreamTemplate::StreamTemplate(const char *str, int size) : str_(str), size_(size) {}
const char *StreamTemplate::GetStr() const { return str_; }
int StreamTemplate::GetSize() const { return size_; }

const char *Formator::GetValue() const { return value_; }
int Formator::GetLen() const { return len_; }

SourceFile::SourceFile(const char *data) : data_(data), size_(static_cast<int>(strlen(data))) {}
