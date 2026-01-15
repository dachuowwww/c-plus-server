#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "LogStream.h"
#include "Macro.h"

static const double BUFFERWRITETIMEOUT = 3.0;               // 等待写入的时间
static const int64_t FILEMAXIMUMSIZE = 1024 * 1024 * 1024;  // 单个文件最大的容量
static const int FIXEDLARGEBUFFFERSIZE = 4096 * 1000;
class Latch;
class AsyncLogging {
 public:
  explicit AsyncLogging(const char *filepath);
  ~AsyncLogging();

  void Start();
  void Stop();
  void Append(const char *logline, int len);
  static void Flush();

 private:
  void ThreadFunc();

  bool running_ = false;
  const char *filepath_ = nullptr;

  std::mutex mutex_;
  std::condition_variable cv_;
  std::unique_ptr<Latch> latch_;

  std::unique_ptr<FixedBuffer<FIXEDLARGEBUFFFERSIZE>> current_buffer_;
  std::unique_ptr<FixedBuffer<FIXEDLARGEBUFFFERSIZE>> next_buffer_;
  std::vector<std::unique_ptr<FixedBuffer<FIXEDLARGEBUFFFERSIZE>>> buffers_;
  std::jthread thread_;
  DISALLOW_COPY_AND_ASSIGN(AsyncLogging);
};

void AsyncLogging::Flush() { fflush(stdout); }
