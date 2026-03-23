#pragma once
#include <functional>
#include <string>
#include "Macro.h"
#include "TimeStamp.h"

class Timer {
 public:
  Timer(TimeStamp timestamp, std::function<void()> &&callback, double interval);
  ~Timer() = default;
  void Run();
  [[nodiscard]] bool IfRepeat() const;
  [[nodiscard]] const TimeStamp &GetTimeStamp() const;
  [[nodiscard]] std::string GetTimeString() const;
  [[nodiscard]] int64_t GetTime() const;
  [[nodiscard]] double GetInterval() const;

  void Restart();

 private:
  TimeStamp timestamp_;
  std::function<void()> callback_;
  double interval_ = 0.0;
  bool repeat_ = false;

  DISALLOW_COPY_AND_ASSIGN(Timer);
};
