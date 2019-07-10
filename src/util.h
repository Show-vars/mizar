#ifndef _H_UTIL_
#define _H_UTIL_

#ifndef max
  #define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
  #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void mdie(const char* format, ...);
void mdebug(const char* format, ...);

void* mmalloc(size_t size);
void msleep(const unsigned int ms);

#endif