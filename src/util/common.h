#ifndef _H_UTIL_COMMON_
#define _H_UTIL_COMMON_

#include <unistd.h>

static inline long long os_getpid(void) {
  return getpid();
}

#endif