#pragma once
#include <Macro.h>
#include <sys/time.h>
#include <cstdint>
#include <string>

const int MICROSECOND_2_SECOND = 1000 * 1000;

class TimeStamp {
 public:
  TimeStamp() = default;
  explicit TimeStamp(int64_t micro_seconds);
  ~TimeStamp() = default;
  TimeStamp(const TimeStamp &) = default;             // 拷贝构造
  TimeStamp &operator=(const TimeStamp &) = default;  // 拷贝赋值
  TimeStamp(TimeStamp &&) = default;                  // 移动构造
  TimeStamp &operator=(TimeStamp &&) = default;       // 移动赋值

  bool operator<(const TimeStamp &timestamp) const { return micro_seconds_ < timestamp.micro_seconds_; }
  bool operator==(const TimeStamp &timestamp) const = default;

  [[nodiscard]] std::string ToFormattedString() const;
  [[nodiscard]] int64_t Time() const;

  static TimeStamp Now();
  static TimeStamp AddTime(TimeStamp timestamp, double seconds);

 private:
  int64_t micro_seconds_ = 0;  // 该类属于平凡可复制类型

  // DISALLOW_COPY_AND_ASSIGN(TimeStamp);
};

inline TimeStamp TimeStamp::Now() {
  struct timeval time {};
  gettimeofday(&time, nullptr);
  return TimeStamp(time.tv_sec * MICROSECOND_2_SECOND + time.tv_usec);
}

inline TimeStamp TimeStamp::AddTime(TimeStamp timestamp, double seconds) {
  int64_t add_micro_seconds = seconds * MICROSECOND_2_SECOND;
  return TimeStamp(timestamp.Time() + add_micro_seconds);
}
