#include "Latch.h"

Latch::Latch(int count) : count_(count) {}

void Latch::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [this]() { return count_ <= 0; });
}

void Latch::Notify() {
  std::unique_lock<std::mutex> lock(mutex_);
  --count_;
  if (count_ <= 0) {
    cv_.notify_all();
  }
}
