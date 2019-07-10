#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <uv.h>
#include "playback.h"
#include "ringbuffer.h"
#include "extend.h"
#include "output/output.h"
#include "util.h"

struct playback_state {
  ringbuffer_t* buffer;

  uv_mutex_t buffer_mutex;
  uv_mutex_t producer_mutex;
  uv_mutex_t consumer_mutex;

  uv_thread_t producer_thread;
  uv_thread_t consumer_thread;
};

struct playback_state state;

static void producer_loop(void* arg) {
  mdebug("[PRODUCER] STARTING UP");

  uint8_t buf[32 * 1024];
  size_t r;
  size_t w;
  size_t space;

  FILE *f = fopen("./test4.wav", "rb");
  fseek(f, 44, SEEK_SET);

  while(1) {
    //mdebug("[PRODUCER] LOOP TICK");

    uv_mutex_lock(&state.buffer_mutex);
    space = ringbuffer_get_space(state.buffer);
    uv_mutex_unlock(&state.buffer_mutex);
    //mdebug("[PRODUCER] BUFFER SPACE %d", space);

    if(space <= 0) {
      msleep(50);
      continue;
    }
    
    r = fread(buf, 1, min(sizeof(buf), space), f);
    //mdebug("[PRODUCER] DECODER READ %d", r);

    if(r <= 0) {
      msleep(50);
      continue;
    }
    
    uv_mutex_lock(&state.buffer_mutex);
    w = ringbuffer_write(state.buffer, buf, r);
    uv_mutex_unlock(&state.buffer_mutex);
    //mdebug("[PRODUCER] BUFFER WRITE %d", w);

  }
}

static void consumer_loop(void* arg) {
  mdebug("[CONSUMER] STARTING UP");

  uint8_t buf[32 * 1024];
  size_t r;
  size_t w;
  size_t available;
  size_t space;

  audio_format_t af = af_endian(0) | af_signed(1) | af_depth(16) |
                      af_rate(44100) | af_channels(2);

  output_device_ops.init();
  output_device_ops.open(af);

  while(1) {
    //mdebug("[CONSUMER] LOOP TICK");

    space = output_device_ops.buffer_space();
    //mdebug("[CONSUMER] OUTPUT SPACE %d", space);

    if(space <= 0) {
      msleep(10);
      continue;
    }
    
    uv_mutex_lock(&state.buffer_mutex);
    available = ringbuffer_get_available(state.buffer);
    uv_mutex_unlock(&state.buffer_mutex);
    //mdebug("[CONSUMER] BUFFER AVAILABLE %d", available);

    if(available <= 0) {
      msleep(10);
      continue;
    }

    uv_mutex_lock(&state.buffer_mutex);
    r = ringbuffer_read(state.buffer, buf, min(sizeof(buf), min(available, space)));
    uv_mutex_unlock(&state.buffer_mutex);
    //mdebug("[CONSUMER] BUFFER READ %d", r);

    w = output_device_ops.write(buf, r);
    //mdebug("[CONSUMER] OUTPUT WRITE %d", w);    
  }

  output_device_ops.close();
  output_device_ops.destroy();
}

void playback_init() {
  state.buffer = ringbuffer_create(512 * 1024);

  uv_mutex_init_recursive(&state.buffer_mutex);
  uv_mutex_init_recursive(&state.producer_mutex);
  uv_mutex_init_recursive(&state.consumer_mutex);

  uv_thread_create(&state.producer_thread, producer_loop, NULL);
  uv_thread_create(&state.consumer_thread, consumer_loop, NULL);

  uv_thread_join(&state.consumer_thread);
}