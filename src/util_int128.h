#ifndef _H_UTIL_INT128_
#define _H_UTIL_INT128_

#include <stdint.h>

typedef struct {
  union {
		uint32_t p[4]; /* little-endian */
		struct {
			uint64_t l; 
			uint64_t h;
		};
	};
} util_uint128_t;

static inline util_uint128_t util_mul_64_64(uint64_t a, uint64_t b) {
  util_uint128_t r;

  uint64_t a_l = (uint64_t)(uint32_t)a;
  uint64_t a_h = a >> 32;
  uint64_t b_l = (uint64_t)(uint32_t)b;
  uint64_t b_h = b >> 32;

  uint64_t p0 = a_l * b_l;
  uint64_t p1 = a_l * b_h;
  uint64_t p2 = a_h * b_l;
  uint64_t p3 = a_h * b_h;

  uint32_t cy = (uint32_t)(((p0 >> 32) + (uint32_t)p1 + (uint32_t)p2) >> 32);

  r.l = p0 + (p1 << 32) + (p2 << 32);
  r.h = p3 + (p1 >> 32) + (p2 >> 32) + cy;

  return r;
}

static inline util_uint128_t util_div_128_64(util_uint128_t a, uint64_t b) {
  util_uint128_t r;

	uint64_t v = 0;

	for (int i = 3; i >= 0; i--) {
		v = (v << 32) | a.p[i];
		if (v < b) {
			r.p[i] = 0;
			continue;
		}

		r.p[i] = (uint32_t)(v / b);
		v = v % b;
	}

	return r;
}

#endif