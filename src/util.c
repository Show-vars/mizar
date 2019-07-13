#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "util.h"

void* mmalloc(size_t size) {
  void* p;
  if ((p = malloc(size)) == NULL) perror("Unable to allocate memory (malloc)");
  return p;
}

void msleep(const unsigned int ms) { usleep(ms * 1000); }
