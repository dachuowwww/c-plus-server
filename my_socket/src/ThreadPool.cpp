#include "ThreadPool.h"
#include <iostream>
#include <stdexcept>  // 包含异常处理相关的头文件
#include <thread>     // 包含线程相关的头文件
#include "Error.h"
using std::cout;
using std::endl;
using std::mutex;
using std::unique_lock;

ThreadPool::ThreadPool(unsigned int size) {
  if (size <= 0) {
    size = 1;
  }
  std::cout << "ThreadPool has started with " << size << " threads" << std::endl;
  threads_.reserve(size);  // 预留内存空间
  Errif(threads_.capacity() < size, "Threads vector reserve failed");
  for (unsigned int i = 0; i < size; ++i) {
    threads_.emplace_back([this] {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(mtx_);
          cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
          if (stop_ && tasks_.empty()) {
            return;
          }
          task = std::move(tasks_.front());
          tasks_.pop();
        }
        task();  // 执行任务
      }
    });
    Errif(!threads_.back().joinable(), "Thread creation failed");
  }
}

ThreadPool::~ThreadPool() {
  ThreadPool::StopThreads();
  cout << "ThreadPool has stopped" << endl;
}

void ThreadPool::StopThreads() {
  {
    unique_lock<mutex> lock(mtx_);
    stop_ = true;
  }
  cv_.notify_all();
}
