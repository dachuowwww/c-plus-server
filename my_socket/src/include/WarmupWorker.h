#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "RedisClient.h"
#include "Macro.h"

class WarmupWorker {
 public:
  using Builder = std::function<std::string()>;

  WarmupWorker() = default;
  ~WarmupWorker();
  static WarmupWorker &Instance();

  void RegisterPage(const std::string &key, int ttl_seconds, Builder builder);
  void Hit(const std::string &target, double score);
  void Start();
  void Stop();
  void Run();

 private:
  struct PageSpec {
    int ttl_seconds = 0;
    Builder builder;
  };

  std::atomic<bool> stop_{false};
  std::thread worker_;
  std::mutex mutex_;
  std::unordered_map<std::string, PageSpec> specs_;
  RedisClient redis_;

  DISALLOW_COPY_AND_ASSIGN(WarmupWorker);
};
