#include "Logger.h"
#include "CurrentThread.h"
#include "TimeStamp.h"

// 为了实现多线程中日志时间格式化的效率，增加了两个__thread变量，
// 用于缓存当前线程存日期时间字符串、上一次日志记录的秒数
__thread char t_time[64];      // 当前线程的时间字符串 “年:月:日 时:分:秒”
__thread time_t t_lastsecond;  // 当前线程上一次日志记录时的秒数

void DefaultOutput(const char *msg, int len) {
  fwrite(msg, 1, len, stdout);  // 默认写出到stdout
}

void DefaultFlush() {
  fflush(stdout);  // 默认flush到stdout 刷新缓冲区才会真正写入
}

Logger::OutputFunc g_output = DefaultOutput;
Logger::FlushFunc g_flush = DefaultFlush;
Logger::LogLevel g_log_level = Logger::LogLevel::INFO;

void Logger::SetOutput(Logger::OutputFunc func) { g_output = func; }
void Logger::SetFlush(Logger::FlushFunc func) { g_flush = func; }

Logger::Logger(const char *file_, int line, LogLevel level) {
  impl_ = std::make_unique<Impl>(std::make_unique<SourceFile>(file_), line, level);
}
Logger::~Logger() = default;
LogStream &Logger::GetStream() { return impl_->GetLogStream(); }

// 全局方法，设置日志全局日志级别，flush输出目的地

Logger::Impl::Impl(std::unique_ptr<SourceFile> &&source, int line, Logger::LogLevel level)
    : level_(level), sourcefile_(std::move(source)), line_(line) {
  FormattedTime();
  current_thread::Tid();
  stream_ << StreamTemplate(current_thread::ToStringTid(), current_thread::LengthTid());
  stream_ << " ";
  stream_ << StreamTemplate(GetLevelString(), 8);  // 头文件需要包含重载运算符的完整体
}

Logger::Impl::~Impl() {
  Finish();
  const FixedBuffer<FIXEDBUFFERSIZE> *buffer(GetLogStream().GetBuffer());
  g_output(buffer->Buffer(), buffer->GetLength());
  if (GetLevel() == FATAL) {
    g_flush();
    abort();
  }
}
void Logger::Impl::FormattedTime() {
  TimeStamp now = TimeStamp::Now();
  time_t seconds = static_cast<time_t>(now.Time() / MICROSECOND_2_SECOND);
  int microseconds = static_cast<int>(now.Time() % MICROSECOND_2_SECOND);

  // 变更日志记录的时间，如果不在同一秒，则更新时间。
  // 方便在同一秒内输出多个日志信息
  if (t_lastsecond != seconds) {
    struct tm tm_time = {};
    localtime_r(&seconds, &tm_time);
    snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d", tm_time.tm_year + 1900, tm_time.tm_mon + 1,
             tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    t_lastsecond = seconds;
  }
  Formator us(".%06d  ", microseconds);
  stream_ << StreamTemplate(t_time, 17) << StreamTemplate(us.GetValue(), 9);
}

void Logger::Impl::Finish() { stream_ << "-" << sourcefile_->data_ << ":" << line_ << "\n"; }

LogStream &Logger::Impl::GetLogStream() { return stream_; }
const char *Logger::Impl::GetLevelString() const {
  switch (level_) {
    case DEBUG:
      return "DEBUG   ";  // 注意要读满八个字节（无\0)
    case INFO:
      return "INFO    ";
    case WARN:
      return "WARN    ";
    case ERROR:
      return "ERROR   ";
    case FATAL:
      return "FATAL   ";
    default:
      return "UNKNOWN ";
      break;
  }
}

void Logger::SetLogLevel(Logger::LogLevel level) { g_log_level = level; }
