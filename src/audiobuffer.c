#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "audiobuffer.h"
#include "pcm.h"
#include "util.h"

audiobuffer_t* audiobuffer_create(audio_format_t af, size_t frames) {
  audiobuffer_t* b;

  size_t capacity = frames * af_get_channels(af);
  b = mmalloc(offsetof(audiobuffer_t, data) + capacity * sizeof(float));
  
  b->af = af;
  b->capacity = capacity;
  b->frames = frames;
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
  b->frames = 0;
  uv_mutex_unlock(&b->mutex);
}

size_t audiobuffer_get_available(audiobuffer_t* b) {
  size_t r, channels;

  uv_mutex_lock(&b->mutex);
  channels = af_get_channels(b->af);
  r = b->available;
  uv_mutex_unlock(&b->mutex);

  return r / channels;
}

size_t audiobuffer_get_space(audiobuffer_t* b) {
  size_t r, channels;
  
  uv_mutex_lock(&b->mutex);
  channels = af_get_channels(b->af);
  r = b->capacity - b->available;
  uv_mutex_unlock(&b->mutex);

  return r / channels;
}

uint64_t audiobuffer_get_frames(audiobuffer_t* b) {
  uint64_t frames;
  
  uv_mutex_lock(&b->mutex);
  frames = b->frames;
  uv_mutex_unlock(&b->mutex);

  return frames;
}

void audiobuffer_set_frames(audiobuffer_t* b, uint64_t frames) {
  uv_mutex_lock(&b->mutex);
  b->frames = frames;
  uv_mutex_unlock(&b->mutex);
}

size_t audiobuffer_read(audiobuffer_t* b, float* dst, const size_t frames) {
  if (frames <= 0) return 0;

  uv_mutex_lock(&b->mutex);

  size_t channels = af_get_channels(b->af);
  size_t samples = frames * channels;
  size_t read_index = b->read_index;
  size_t available = b->available;
  size_t capacity = b->capacity;
  float* src = b->data;
  
  uv_mutex_unlock(&b->mutex);

  if (available <= 0) return 0;

  size_t limit, count, count_end;

  limit = min(read_index + available, capacity);
  count = min(limit - read_index, samples);

  memcpy(dst, src + read_index, count * sizeof(float));

  read_index += count;

  if (read_index >= capacity) {
    count_end = min(samples - count, available - count);

    if (count_end > 0) {
      memcpy(dst + count, src, count_end * sizeof(float));

      count += count_end;
    }
  }

  return count / channels;
}

size_t audiobuffer_consume(audiobuffer_t* b, const size_t frames) {
  uv_mutex_lock(&b->mutex);

  size_t samples = frames * af_get_channels(b->af);

  b->read_index = (b->read_index + samples) % b->capacity;
  b->available -= samples;
  b->frames += frames;

  uv_mutex_unlock(&b->mutex);

  return frames;
}

size_t audiobuffer_write(audiobuffer_t* b, float* src, const size_t frames) {
  if (frames <= 0) return 0;

  uv_mutex_lock(&b->mutex);

  size_t channels = af_get_channels(b->af);
  size_t samples = frames * channels;
  size_t write_index = b->write_index;
  size_t available = b->available;
  size_t capacity = b->capacity;
  float* dst = b->data;
  
  uv_mutex_unlock(&b->mutex);

  if (available >= capacity) return 0;

  size_t limit, count, count_end;
  limit = min(write_index + capacity - available, capacity);
  count = min(limit - write_index, samples);
  
  memcpy(dst + write_index, src, count * sizeof(float));

  write_index += count;

  if (write_index == capacity) {
    count_end = min(samples - count, capacity - available - count);

    if (count_end > 0) {
      memcpy(dst, src + count, count_end * sizeof(float));
      count += count_end;
    }
  }

  return count / channels;
}

size_t audiobuffer_fill(audiobuffer_t* b, const size_t frames) {
  uv_mutex_lock(&b->mutex);

  size_t samples = frames * af_get_channels(b->af);

  b->write_index = (b->write_index + samples) % b->capacity;
  b->available += samples;

  uv_mutex_unlock(&b->mutex);

  return frames;
}