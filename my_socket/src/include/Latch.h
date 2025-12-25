#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>
#include "Macro.h"

class Latch {
 public:
  explicit Latch(int count);
  ~Latch() = default;

  void Wait();
  void Notify();

 private:
  int count_;
  std::mutex mutex_;
  std::condition_variable cv_;

  DISALLOW_COPY_AND_ASSIGN(Latch);
};
