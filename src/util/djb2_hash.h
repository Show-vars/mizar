#ifndef _H_UTIL_DJB2_
#define _H_UTIL_DJB2_

// djb2 hash implementation by Dan Bernstein
// http://www.cse.yorku.ca/~oz/hash.html
static inline unsigned djb2_hash(const char* str) {
  unsigned hash = 5381;
  while (*str) hash = ((hash << 5) + hash) ^ *str++;
  return hash;
}

#endif