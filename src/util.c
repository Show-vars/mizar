#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

void mdie(const char* format, ...) {
  va_list argptr;
  va_start(argptr, format);
  vfprintf(stderr, format, argptr);
  fprintf(stdout, "\n");
  va_end(argptr);

  exit(1);
}

void mdebug(const char* format, ...) {
  va_list argptr;
  va_start(argptr, format);
  fprintf(stdout, "[DEBUG] ");
  vfprintf(stdout, format, argptr);
  fprintf(stdout, "\n");
  va_end(argptr);
}

void* mmalloc(size_t size) {
  void* p;
  if ((p = malloc(size)) == NULL) mdie("Unable to allocate memory.");
  return p;
}

void msleep(const unsigned int ms) { usleep(ms * 1000); }
