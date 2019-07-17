#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "audiobuffer.h"
#include "pcm.h"
#include "util.h"

audiobuffer_t* audiobuffer_create(audio_format_t af, size_t capacity) {
  audiobuffer_t* b;

  size_t size = capacity * af_get_channels(af);
  b = mmalloc(offsetof(audiobuffer_t, data) + sizeof(float) * size);
  
  b->af = af;
  b->size = size;
  b->capacity = capacity;
  b->available = b->read_index = b->write_index = 0;

  uv_mutex_init_recursive(&b->mutex);
  
  return b;
}

void audiobuffer_destroy(audiobuffer_t* b) {
  uv_mutex_destroy(&b->mutex);
  
  free(b);
}

void audiobuffer_reset(audiobuffer_t* b) {
  uv_mutex_lock(&b->mutex);
  b->available = b->read_index = b->write_index = 0;
  b->time = 0;
  uv_mutex_unlock(&b->mutex);
}

size_t audiobuffer_get_available(audiobuffer_t* b) {
  size_t r;

  uv_mutex_lock(&b->mutex);
  r = b->available;
  uv_mutex_unlock(&b->mutex);

  return r;
}

size_t audiobuffer_get_space(audiobuffer_t* b) {
  size_t r;
  
  uv_mutex_lock(&b->mutex);
  r = b->capacity - b->available;
  uv_mutex_unlock(&b->mutex);

  return r;
}

double audiobuffer_get_time(audiobuffer_t* b) {
  double time;
  
  uv_mutex_lock(&b->mutex);
  time = b->time;
  uv_mutex_unlock(&b->mutex);

  return time;
}

void audiobuffer_set_time(audiobuffer_t* b, double time) {
  uv_mutex_lock(&b->mutex);
  b->time = time;
  uv_mutex_unlock(&b->mutex);
}

size_t audiobuffer_read(audiobuffer_t* b, float* dst, const size_t len) {
  if (len <= 0) return 0;

  size_t limit, count, count_end;

  uv_mutex_lock(&b->mutex);

  size_t channels = af_get_channels(b->af);
  size_t read_index = b->read_index;
  size_t available = b->available;
  size_t capacity = b->capacity;
  float* src = b->data;
  
  uv_mutex_unlock(&b->mutex);

  if (available < len) return 0;

  limit = min(read_index + available, capacity);
  count = min(limit - read_index, len);

  memcpy(dst, src + read_index * channels, count * channels);

  read_index += count;

  if (read_index >= capacity) {
    count_end = min(len - count, available - count);

    if (count_end > 0) {
      memcpy(dst + count * channels, src, count_end * channels);

      count += count_end;
    }
  }

  return count;
}

size_t audiobuffer_consume(audiobuffer_t* b, const size_t count) {
  uv_mutex_lock(&b->mutex);

  b->read_index = (b->read_index + count) % b->capacity;
  b->available -= count;
  b->time += (double) count / af_get_second_size(b->af);

  uv_mutex_unlock(&b->mutex);

  return count;
}

size_t audiobuffer_write(audiobuffer_t* b, float* src, const size_t len) {
  if (len <= 0) return 0;

  size_t limit, count, count_end;

  uv_mutex_lock(&b->mutex);

  size_t channels = af_get_channels(b->af);
  size_t write_index = b->write_index;
  size_t available = b->available;
  size_t capacity = b->capacity;
  float* dst = b->data;
  
  uv_mutex_unlock(&b->mutex);

  if (available >= capacity) return 0;

  limit = min(write_index + capacity - available, capacity);
  count = min(limit - write_index, len);
  

  memcpy(dst + write_index * channels, src, count * channels);

  write_index += count;

  if (write_index == capacity) {
    count_end = min(len - count, capacity - available - count);

    if (count_end > 0) {
      memcpy(dst, src + count * channels, count_end * channels);
      count += count_end;
    }
  }

  return count;
}

size_t audiobuffer_fill(audiobuffer_t* b, const size_t count) {
  uv_mutex_lock(&b->mutex);

  b->write_index = (b->write_index + count) % b->capacity;
  b->available += count;

  uv_mutex_unlock(&b->mutex);

  return count;
}