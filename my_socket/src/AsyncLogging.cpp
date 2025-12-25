#include "AsyncLogging.h"
#include "Latch.h"
#include "LogFile.h"

AsyncLogging::AsyncLogging(const char *filepath) : filepath_(filepath), latch_(std::make_unique<Latch>(1)) {
  current_buffer_ = std::make_unique<FixedBuffer<FIXEDLARGEBUFFFERSIZE>>();
  next_buffer_ = std::make_unique<FixedBuffer<FIXEDLARGEBUFFFERSIZE>>();
  buffers_ = std::vector<std::unique_ptr<FixedBuffer<FIXEDLARGEBUFFFERSIZE>>>();
}

AsyncLogging::~AsyncLogging() {
  if (running_) {
    Stop();
  }
}

void AsyncLogging::Start() {
  running_ = true;
  thread_ = std::jthread(&AsyncLogging::ThreadFunc, this);
  latch_->Wait();  // 等待日志线程启动完成
}

void AsyncLogging::Stop() {
  running_ = false;
  cv_.notify_all();
}

void AsyncLogging::Append(const char *logline, int len) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (current_buffer_->GetSpace() < len) {
    buffers_.emplace_back(std::move(current_buffer_));
    if (next_buffer_) {
      current_buffer_ = std::move(next_buffer_);
    } else {
      current_buffer_ = std::make_unique<FixedBuffer<FIXEDLARGEBUFFFERSIZE>>();
    }
  }
  current_buffer_->Append(logline, len);
  cv_.notify_all();
}

void AsyncLogging::ThreadFunc() {  // 后端日志线程函数
  latch_->Notify();                // 通知主线程，日志线程已经启动完成
  std::unique_ptr<FixedBuffer<FIXEDLARGEBUFFFERSIZE>> new_current =
      std::make_unique<FixedBuffer<FIXEDLARGEBUFFFERSIZE>>();
  std::unique_ptr<FixedBuffer<FIXEDLARGEBUFFFERSIZE>> new_next = std::make_unique<FixedBuffer<FIXEDLARGEBUFFFERSIZE>>();

  new_current->Clear();
  new_next->Clear();

  std::unique_ptr<LogFile> log_file = std::make_unique<LogFile>(filepath_);
  std::vector<std::unique_ptr<FixedBuffer<FIXEDLARGEBUFFFERSIZE>>> active_buffers;
  while (running_) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (buffers_.empty()) {
        cv_.wait_until(lock, std::chrono::system_clock::now() + BUFFERWRITETIMEOUT * std::chrono::milliseconds(1000),
                       []() { return false; });
      }
      buffers_.emplace_back(std::move(current_buffer_));
      current_buffer_ = std::move(new_current);
      active_buffers.swap(buffers_);
      if (!next_buffer_) {
        next_buffer_ = std::move(new_next);
      }
    }

    for (const auto &buffer : active_buffers) {
      if (log_file->WrittenBytes() >= FILEMAXIMUMSIZE) {
        log_file = std::make_unique<LogFile>(filepath_);
      }
      log_file->Write(buffer->Buffer(), buffer->GetLength());
    }

    if (active_buffers.size() > 2){
        active_buffers.resize(2);  // 换来的缓冲区进行重用
    }
    

    new_current = std::move(active_buffers.back());
    new_current->Clear();
    active_buffers.pop_back();

    if (!new_next) {
      new_next = std::move(active_buffers.back());
      new_next->Clear();
      active_buffers.pop_back();
    }
    active_buffers.clear();
  }
}
