#include "Error.h"
#include <cstdio>
#include <cstdlib>

void Errif(bool condition, const char *errmsg) {
  if (condition) {
    perror(errmsg);
    exit(EXIT_FAILURE);
  }
}
