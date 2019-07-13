#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <uv.h>
#include "playback.h"
#include "ringbuffer.h"
#include "extend.h"
#include "output/output.h"
#include "decoder/decoder.h"
#include "decoder/decoder_impl.h"
#include "log.h"
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
audio_format_t af = af_endian(0) | af_signed(1) | af_depth(16) |
                      af_rate(44100) | af_channels(2);

static void producer_loop(void* arg) {
  log_ddebug("[PRODUCER] STARTING UP");

  uint8_t buf[32 * 1024];
  size_t r;
  size_t w;
  size_t space;
  decoder_data_t decoder_data;
  decoder_data.af = af;

  int rc;
  rc = decoder_mp3.open(&decoder_data);
  if (rc < 0) {
    log_error("Unable to open decoder");
    return;
  }

  while(1) {
    uv_mutex_lock(&state.buffer_mutex);
    space = ringbuffer_get_space(state.buffer);
    uv_mutex_unlock(&state.buffer_mutex);

    if(space <= 0) {
      msleep(50);
      continue;
    }

    log_dtrace("[PRODUCER] %d bytes free in ring buffer", space);
    
    r = decoder_mp3.read(&decoder_data, buf, min(sizeof(buf), space));

    if(r <= 0) {
      msleep(50);
      continue;
    }
    
    log_dtrace("[PRODUCER] %d bytes read", r);
    
    uv_mutex_lock(&state.buffer_mutex);
    w = ringbuffer_write(state.buffer, buf, r);
    uv_mutex_unlock(&state.buffer_mutex);

    if(w <= 0) {
      //Something gone wrong
      msleep(50); //In any strange situation, go to sleep
    }

    log_dtrace("[PRODUCER] %d bytes written", w);
  }
}

static void consumer_loop(void* arg) {
  log_ddebug("[CONSUMER] STARTING UP");

  uint8_t buf[32 * 1024];
  size_t r;
  size_t w;
  size_t available;
  size_t space;

  int rc;
  rc = output_device_ops.init();
  if (rc < 0) {
    log_error("Unable to initialize output audio device");
    return;
  }

  rc = output_device_ops.open(af);
  if (rc < 0) {
    log_error("Unable to open output audio device");
    return;
  }

  while(1) {
    space = output_device_ops.buffer_space();

    if(space <= 0) {
      msleep(10);
      continue;
    }

    log_dtrace("[PRODUCER] %d bytes free in output buffer", space);
    
    uv_mutex_lock(&state.buffer_mutex);
    available = ringbuffer_get_available(state.buffer);
    uv_mutex_unlock(&state.buffer_mutex);

    if(available <= 0) {
      msleep(10);
      continue;
    }

    log_dtrace("[PRODUCER] %d bytes available in ring buffer", available);

    uv_mutex_lock(&state.buffer_mutex);
    r = ringbuffer_read(state.buffer, buf, min(sizeof(buf), min(available, space)));
    uv_mutex_unlock(&state.buffer_mutex);

    log_dtrace("[PRODUCER] %d bytes readed from ring buffer", r);

    w = output_device_ops.write(buf, r);

    if(w <= 0) {
      msleep(50);
    }
    
    log_dtrace("[PRODUCER] %d bytes written to output device", w);
  }

  output_device_ops.close();
  output_device_ops.destroy();
}

void playback_init() {
  state.buffer = ringbuffer_create(512 * 1024);

  int rc;

  rc = uv_mutex_init_recursive(&state.buffer_mutex);
  if (rc < 0) {
    log_fatal("uv_mutex_init_recursive(buffer_mutex)");
    exit(EXIT_FAILURE);
  }

  rc = uv_mutex_init_recursive(&state.producer_mutex);
  if (rc < 0) {
    log_fatal("uv_mutex_init_recursive(producer_mutex)");
    exit(EXIT_FAILURE);
  }

  rc = uv_mutex_init_recursive(&state.consumer_mutex);
  if (rc < 0) {
    log_fatal("uv_mutex_init_recursive(consumer_mutex)");
    exit(EXIT_FAILURE);
  }

  rc = uv_thread_create(&state.producer_thread, producer_loop, NULL);
  if (rc < 0) {
    log_fatal("uv_thread_create(producer_thread)");
    exit(EXIT_FAILURE);
  }

  rc = uv_thread_create(&state.consumer_thread, consumer_loop, NULL);
  if (rc < 0) {
    log_fatal("uv_thread_create(consumer_thread)");
    exit(EXIT_FAILURE);
  }

  rc = uv_thread_join(&state.consumer_thread);
  if (rc < 0) {
    log_fatal("uv_thread_join(consumer_thread)");
    exit(EXIT_FAILURE);
  }
}