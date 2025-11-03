#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
 private:
  bool stop_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::thread> threads_;

 public:
  explicit ThreadPool(unsigned int size = std::thread::hardware_concurrency());
  ~ThreadPool();
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;
  void StopThreads();

  template <typename F, typename... Args>
  auto Add(F &&f, Args &&... args)
      -> std::future<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
    using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
    auto task =
        std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    auto res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(mtx_);
      if (stop_) throw std::runtime_error("ThreadPool has been stopped");
      tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return res;
  }
};
