#pragma once
#include <sys/time.h>
#include <cstdio>
#include "Macro.h"
#include "TimeStamp.h"
static const time_t FLUSHINTERVAL = 3;

class LogFile {
 public:
  explicit LogFile(const char *filename);
  ~LogFile();

  void Write(const char *message, int len);  // 写入日志文件
  [[nodiscard]] int WrittenBytes() const;

  void Flush();  // 输出到屏幕
 private:
  FILE *file_ = nullptr;
  int written_bytes_ = 0;
  TimeStamp last_flush_time_ = TimeStamp::Now();
  TimeStamp last_write_time_ = TimeStamp::Now();

  DISALLOW_COPY_AND_ASSIGN(LogFile);
};
