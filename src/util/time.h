#ifndef _H_UTIL_TIME_
#define _H_UTIL_TIME_

#include <time.h>
#include <sys/times.h>

static inline void os_sleep(int sec, int ns) {
  struct timespec req;
  req.tv_sec = sec;
  req.tv_nsec = ns;
  nanosleep(&req, NULL);
}

static inline uint64_t os_gettime_ns(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec);
}

#define os_sleep_ns(ns) os_sleep(0, (ns))
#define os_sleep_us(us) os_sleep_ns((us) * 1e3)
#define os_sleep_ms(us) os_sleep_ns((us) * 1e6)
#define os_sleep_sc(sc) os_sleep((sc), 0)

#endif