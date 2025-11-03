#include "Error.h"
#include <stdio.h>
#include <stdlib.h>

void Errif(bool condition, const char *errmsg) {
  if (condition) {
    perror(errmsg);
    exit(EXIT_FAILURE);
  }
}
