#include <TimeStamp.h>
#include <ctime>

TimeStamp::TimeStamp(int64_t micro_seconds) { micro_seconds_ = micro_seconds; }

std::string TimeStamp::ToFormattedString() const {
  char buf[64] = {0};
  time_t seconds = static_cast<time_t>(micro_seconds_ / MICROSECOND_2_SECOND);
  struct tm tm_time {};
  localtime_r(&seconds, &tm_time);  // 将秒转换为本地时间
  int microseconds = static_cast<int>(micro_seconds_ % MICROSECOND_2_SECOND);
  snprintf(buf, sizeof(buf), "%4d-%02d-%02d_%02d:%02d:%02d.%06d", tm_time.tm_year + 1900, tm_time.tm_mon + 1,
           tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
  return buf;
}

int64_t TimeStamp::Time() const { return micro_seconds_; }
