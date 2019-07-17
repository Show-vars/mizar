#ifndef _H_UTIL_
#define _H_UTIL_

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#ifndef max
  #define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
  #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef clamp
  #define clamp(val,minval,maxval) ((val > maxval) ? maxval : ((val < minval) ? minval : val))
#endif

static inline void* mmalloc(size_t size) {
  void* p;
  if ((p = malloc(size)) == NULL) perror("Unable to allocate memory (malloc)");
  return p;
}

static inline void nano_sleep(int sec, int ns) {
  struct timespec req;
  req.tv_sec = sec;
  req.tv_nsec = ns;
  nanosleep(&req, NULL);
}

#define ns_sleep(us)   nano_sleep(0, (us))
#define us_sleep(us)   ns_sleep((us) * 1e3)
#define ms_sleep(us)   ns_sleep((us) * 1e6)
#define sec_sleep(sec) nano_sleep((sec), 0)

// djb2 hash implementation by Dan Bernstein
// http://www.cse.yorku.ca/~oz/hash.html
static inline unsigned djb2_hash(const char* str) {
  unsigned hash = 5381;
  while (*str) hash = ((hash << 5) + hash) ^ *str++;
  return hash;
}

#endif