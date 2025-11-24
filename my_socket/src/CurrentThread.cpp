#include "CurrentThread.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdio>

namespace current_thread {
__thread int tid = 0;
__thread char str_tid[32] = {0};
__thread int len_tid = 0;

pid_t GetTid() { return static_cast<int>(syscall(SYS_gettid)); }
void CacheTid() {
  if (tid == 0) {
    tid = GetTid();
    len_tid = snprintf(str_tid, sizeof(str_tid), "%5d", tid);
  }
}
}  // namespace current_thread
