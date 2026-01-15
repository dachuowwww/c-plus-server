#include "LogFile.h"
#include <string>
#include "Error.h"

LogFile::LogFile(const char *filename) {
  file_ = fopen(filename, "a+");
  if (!file_) {
    std::string default_path = "../LogFiles/LogFile_" + TimeStamp::Now().ToFormattedString() + ".log";
    file_ = fopen(default_path.c_str(), "a+");
  }
}

LogFile::~LogFile() {
  Flush();
  if (file_) {
    fclose(file_);
  }
}

void LogFile::Write(const char *message, int len) {
  size_t pos = 0;
  while (pos < static_cast<size_t>(len)) {
    pos += fwrite_unlocked(message + pos, sizeof(char), len - pos, file_);
  }
  if (len != 0) {
    written_bytes_ += len;
    last_write_time_ = TimeStamp::Now();
  }

  if (TimeStamp::AddTime(last_flush_time_, FLUSHINTERVAL) < last_write_time_) {
    Flush();
    last_flush_time_ = TimeStamp::Now();
  }
}

int LogFile::WrittenBytes() const { return written_bytes_; }

void LogFile::Flush() {
  if (file_) {
    fflush(file_);
  }
}
