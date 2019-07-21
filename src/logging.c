#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logging.h"

struct {
  unsigned maxlevel;
} L;

static const char *level_names[] = { "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE" };

static void log_log(unsigned level, const char *prefix, const char *fmt, va_list arg) {
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);

  char buf[20];
  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';

  fprintf(stdout, "%s [%-5s]", buf, level_names[level]);
  if(strlen(prefix) > 0) fprintf(stdout, " [%s]", prefix);
  fprintf(stdout, ": ");
  vfprintf(stdout, fmt, arg);
  fprintf(stdout, "\n");
}

void log_init(unsigned maxlevel) {
  L.maxlevel = maxlevel;
}

void log_write(unsigned level, const char *prefix, const char *fmt, ...) {
  if(level > L.maxlevel || level >= sizeof(level_names)) return;

  va_list arg;
  va_start(arg, fmt);

  log_log(level, prefix, fmt, arg);

  va_end(arg);
}

void log_writev(unsigned level, const char *prefix, const char *fmt, va_list arg) {
  if(level > L.maxlevel || level >= sizeof(level_names)) return;
  
  log_log(level, prefix, fmt, arg);
}

void log_write_direct(const char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);

  vfprintf(stdout, fmt, arg);
  fprintf(stdout, "\n");

  va_end(arg);
}