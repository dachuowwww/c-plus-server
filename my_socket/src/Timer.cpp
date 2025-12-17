#include "Timer.h"

Timer::Timer(TimeStamp timestamp, std::function<void()> &&callback, double interval)
    : timestamp_(timestamp), callback_(std::move(callback)), interval_(interval), repeat_(interval > 0.0) {}

void Timer::Run() { callback_(); }

bool Timer::IfRepeat() const { return repeat_; }

const TimeStamp &Timer::GetTimeStamp() const { return timestamp_; }

std::string Timer::GetTimeString() const { return timestamp_.ToFormattedString(); }

int64_t Timer::GetTime() const { return timestamp_.Time(); }

double Timer::GetInterval() const { return interval_; }

void Timer::Restart() { timestamp_ = TimeStamp::AddTime(TimeStamp::Now(), interval_); }
