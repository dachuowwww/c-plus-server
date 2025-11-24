#pragma once

#include <pthread.h>
#include <cstdint>

namespace current_thread {
extern __thread int tid;
extern __thread char str_tid[32];
extern __thread int len_tid;

void CacheTid();

pid_t GetTid();

inline pid_t Tid() {
  if (__builtin_expect(tid == 0, 0)) {
    CacheTid();
  }
  return tid;
}

inline const char *ToStringTid() { return str_tid; }
inline int LengthTid() { return len_tid; }
}  // namespace current_thread
