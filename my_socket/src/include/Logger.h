#pragma once
#include <memory>
#include "LogStream.h"  // 显式调用重载运算符
#include "Macro.h"

class SourceFile;
class Logger {
 public:
  enum LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };
  // 构造函数，主要是用于构造Impl
  Logger(const char *file_, int line, LogLevel level);
  ~Logger();
  LogStream &GetStream();

  // 全局方法，设置日志全局日志级别，flush输出目的地
  static LogLevel GetLevel();               // 初始化时就要使用就要设置为static
  static void SetLogLevel(LogLevel level);  // 需要访问一个静态类成员

  using OutputFunc = void (*)(const char *data, int len);  // 定义函数指针
  using FlushFunc = void (*)();
  // 默认fwrite到stdout
  static void SetOutput(Logger::OutputFunc func);
  // 默认fflush到stdout
  static void SetFlush(Logger::FlushFunc func);

 private:
  class Impl {
   public:
    Impl(std::unique_ptr<SourceFile> &&source, int line, Logger::LogLevel level);
    ~Impl();
    void FormattedTime();  // 格式化时间信息
    void Finish();         // 完成格式化，并补充输出源码文件和源码位置

    LogStream &GetLogStream();
    [[nodiscard]] const char *GetLevelString() const;  // 获取LogLevel的字符串

   private:
    LogLevel level_;                          // 日志级别
    std::unique_ptr<SourceFile> sourcefile_;  // 源代码名称
    int line_;                                // 源代码行数

    LogStream stream_;  // 日志缓存流

    DISALLOW_COPY_AND_ASSIGN(Impl);
  };

  std::unique_ptr<Impl> impl_;
  DISALLOW_COPY_AND_ASSIGN(Logger);
};

// 全局的日志级别，静态成员函数定义，静态成员函数实现
extern Logger::LogLevel g_log_level;
inline Logger::LogLevel Logger::GetLevel() { return g_log_level; }

// 日志宏
#define LOG_DEBUG \
  if (Logger::GetLevel() <= Logger::DEBUG) Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).GetStream()
#define LOG_INFO \
  if (Logger::GetLevel() <= Logger::INFO) Logger(__FILE__, __LINE__, Logger::INFO).GetStream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).GetStream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).GetStream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).GetStream()
