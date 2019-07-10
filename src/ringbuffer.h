#ifndef _H_RINGBUFFER_
#define _H_RINGBUFFER_

#include <stdint.h>

typedef struct {
  size_t capacity;

  size_t available;
  size_t read_index;
  size_t write_index;

  uint8_t data[];
} ringbuffer_t;

ringbuffer_t* ringbuffer_create(const size_t capacity);

void ringbuffer_destroy(ringbuffer_t* b);

void ringbuffer_reset(ringbuffer_t* b);

size_t ringbuffer_get_available(ringbuffer_t* b);

size_t ringbuffer_get_space(ringbuffer_t* b);

size_t ringbuffer_read(ringbuffer_t* b, uint8_t* dst, const size_t len);

size_t ringbuffer_write(ringbuffer_t* b, uint8_t* src, const size_t len);

#endif