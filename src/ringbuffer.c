#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "ringbuffer.h"
#include "util.h"

ringbuffer_t* ringbuffer_create(const size_t capacity) {
  ringbuffer_t* b;

  b = mmalloc(offsetof(ringbuffer_t, data) + capacity);

  b->capacity = capacity;
  b->available = b->read_index = b->write_index = 0;

  uv_mutex_init_recursive(&b->mutex);
  
  return b;
}

void ringbuffer_destroy(ringbuffer_t* b) {
  uv_mutex_destroy(&b->mutex);
  
  free(b);
}

void ringbuffer_reset(ringbuffer_t* b) {
  uv_mutex_lock(&b->mutex);
  b->available = b->read_index = b->write_index = 0;
  uv_mutex_unlock(&b->mutex);
}

size_t ringbuffer_get_available(ringbuffer_t* b) {
  size_t r;

  uv_mutex_lock(&b->mutex);
  r = b->available;
  uv_mutex_unlock(&b->mutex);

  return r;
}

size_t ringbuffer_get_space(ringbuffer_t* b) {
  size_t r;
  
  uv_mutex_lock(&b->mutex);
  r = b->capacity - b->available;
  uv_mutex_unlock(&b->mutex);

  return r;
}

size_t ringbuffer_read(ringbuffer_t* b, uint8_t* dst, const size_t len) {
  size_t limit, count, count_end;

  uv_mutex_lock(&b->mutex);

  if (len <= 0 || b->available <= 0) return 0;

  limit = min(b->read_index + b->available, b->capacity);
  count = min(limit - b->read_index, len);

  memcpy(dst, b->data + b->read_index, count);

  b->read_index += count;

  if (b->read_index == b->capacity) {
    count_end = min(len - count, b->available - count);

    if (count_end > 0) {
      memcpy(dst + count, b->data, count_end);

      b->read_index = count_end;
      count += count_end;
    } else {
      b->read_index = 0;
    }
  }

  b->available -= count;

  uv_mutex_unlock(&b->mutex);

  return count;
}

size_t ringbuffer_write(ringbuffer_t* b, uint8_t* src, const size_t len) {
  size_t limit, count, count_end;

  uv_mutex_lock(&b->mutex);

  if (len <= 0 || b->available >= b->capacity) return 0;

  limit = min(b->write_index + b->capacity - b->available, b->capacity);
  count = min(limit - b->write_index, len);

  memcpy(b->data + b->write_index, src, count);

  b->write_index += count;

  if (b->write_index == b->capacity) {
    count_end = min(len - count, b->capacity - b->available - count);

    if (count_end > 0) {
      memcpy(b->data, src + count, count_end);

      b->write_index = count_end;
      count += count_end;
    } else {
      b->write_index = 0;
    }
  }

  b->available += count;

  uv_mutex_unlock(&b->mutex);

  return count;
}