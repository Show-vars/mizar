#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "log.h"

static const char *level_names[] = { "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE" };

void log_init(void) {
  return;
}

void log_write(unsigned level, const char *fmt, ...) {
  if(level >= sizeof(level_names)) return;

  va_list argptr;
  va_start(argptr, fmt);

  log_writev(level, fmt, argptr);

  va_end(argptr);
}

void log_writev(unsigned level, const char *fmt, va_list argptr) {
  if(level >= sizeof(level_names)) return;
  
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);

  char buf[16];
  buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';

  fprintf(stdout, "%s [%-5s]: ", buf, level_names[level]);
  vfprintf(stdout, fmt, argptr);
  fprintf(stdout, "\n");
}

void log_write_direct(const char *fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);

  vfprintf(stdout, fmt, argptr);
  fprintf(stdout, "\n");

  va_end(argptr);
}