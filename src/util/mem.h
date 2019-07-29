#ifndef _H_UTIL_MEM_
#define _H_UTIL_MEM_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// bytes swap
#define BSWAP_16(x) ((((x) & 0x00FF) << 8) | \
                    (((x) & 0xFF00) >> 8))

#define BSWAP_24(x) ((((x) & 0x000000FF) << 16) | \
                    (((x) & 0x0000FF00)) | \
                    (((x) & 0x00FF0000) >> 16))

#define BSWAP_32(x) ((((x) & 0x000000FF) << 24) | \
                    (((x) & 0x0000FF00) << 8) | \
                    (((x) & 0x00FF0000) >> 8) | \
                    (((x) & 0xFF000000) >> 24))

#ifdef WORDS_BIGENDIAN
  #define BSWAP_BTON_16(x) (x)
  #define BSWAP_BTON_32(x) (x)
#else
  #define BSWAP_BTON_16(x) BSWAP_16(x)
  #define BSWAP_BTON_32(x) BSWAP_32(x)
#endif

static inline void* pmalloc(size_t size) {
  void* p;
  if ((p = malloc(size)) == NULL) perror("Unable to allocate memory (malloc)");
  return p;
}

static inline void* zalloc(size_t size) {
	void *mem = malloc(size);
	if (mem) memset(mem, 0, size);
	return mem;
}

#endif