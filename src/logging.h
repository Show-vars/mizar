#ifndef _H_LOGGING_
#define _H_LOGGING_

#include <stdarg.h>
#include <config.h>

#define MIZAR_LOGLEVEL_FATAL  0
#define MIZAR_LOGLEVEL_ERROR  1
#define MIZAR_LOGLEVEL_WARN   2
#define MIZAR_LOGLEVEL_INFO   3
#define MIZAR_LOGLEVEL_DEBUG  4
#define MIZAR_LOGLEVEL_TRACE  5

#define logging(level,prefix,...)  log_write((level), (prefix), __VA_ARGS__)
#define log_fatal(...)  logging(MIZAR_LOGLEVEL_FATAL, "", __VA_ARGS__)
#define log_error(...)  logging(MIZAR_LOGLEVEL_ERROR, "", __VA_ARGS__)
#define log_warn(...)   logging(MIZAR_LOGLEVEL_WARN,  "", __VA_ARGS__)
#define log_info(...)   logging(MIZAR_LOGLEVEL_INFO,  "", __VA_ARGS__)
#define log_debug(...)  logging(MIZAR_LOGLEVEL_DEBUG, "", __VA_ARGS__)
#define log_trace(...)  logging(MIZAR_LOGLEVEL_TRACE, "", __VA_ARGS__)


#ifdef DEVEL_LOGGING_ENABLED
  #define log_dfatal(...)  logging(MIZAR_LOGLEVEL_FATAL, "", __VA_ARGS__)
  #define log_derror(...)  logging(MIZAR_LOGLEVEL_ERROR, "", __VA_ARGS__)
  #define log_dwarn(...)   logging(MIZAR_LOGLEVEL_WARN,  "", __VA_ARGS__)
  #define log_dinfo(...)   logging(MIZAR_LOGLEVEL_INFO,  "", __VA_ARGS__)
  #define log_ddebug(...)  logging(MIZAR_LOGLEVEL_DEBUG, "", __VA_ARGS__)
  #define log_dtrace(...)  logging(MIZAR_LOGLEVEL_TRACE, "", __VA_ARGS__)
#else
  #define log_dfatal(...)
  #define log_derror(...)
  #define log_dwarn(...)
  #define log_dinfo(...)
  #define log_ddebug(...)
  #define log_dtrace(...)
#endif

void log_init(unsigned maxlevel);
void log_write(unsigned level, const char *prefix, const char *fmt, ...);
void log_writev(unsigned level, const char *prefix, const char *fmt, va_list arg);
void log_write_direct(const char *fmt, ...);

#endif