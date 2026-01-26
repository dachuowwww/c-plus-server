#include "include/Error.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include "Logger.h"

void Errif(bool condition, const char *errmsg) {
  if (condition) {
    int err = errno;
    perror(errmsg);
    LOG_ERROR << errmsg << " errno=" << err << " msg=" << strerror(err);
  }
}
