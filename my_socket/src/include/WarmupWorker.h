#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "Macro.h"

class WarmupWorker {
 public:
  WarmupWorker() = default;
  ~WarmupWorker();
  static WarmupWorker &Instance();

  void RegisterPage(const std::string &key, int ttl_seconds);
  void Hit(const std::string &target, double score);
  void Start();
  void Stop();
  void Run();

 private:
  void RefreshHotPages();

  std::atomic<bool> stop_{false};
  std::thread worker_;
  std::mutex mutex_;
  std::unordered_map<std::string, int> specs_;

  DISALLOW_COPY_AND_ASSIGN(WarmupWorker);
};
