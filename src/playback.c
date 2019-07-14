#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <uv.h>
#include "playback.h"
#include "ringbuffer.h"
#include "commandqueue.h"
#include "extend.h"
#include "output/output.h"
#include "decoder/decoder.h"
#include "log.h"
#include "util.h"

struct playback_state {
  ringbuffer_t* buffer;
  command_queue_t* cmdqueue;

  decoder_ops_t decoder_ops;
  decoder_data_t decoder_data;

  uv_mutex_t producer_mutex;
  uv_mutex_t consumer_mutex;

  int producer_enabled;
  int consumer_enabled;

  int producer_ready;
  int consumer_ready;

  uv_cond_t producer_cond;
  uv_cond_t consumer_cond;

  uv_thread_t control_thread;
  uv_thread_t producer_thread;
  uv_thread_t consumer_thread;
};

struct playback_state state;
audio_format_t af = af_endian(0) | af_signed(1) | af_depth(16) |
                      af_rate(44100) | af_channels(2);

static void control_loop(void* arg) {
  log_ddebug("[CONTROL ] STARTING UP");

  int controller_ready = 0;

  int rc;

  command_t cmd;
  while(1) {
    command_queue_poll(state.cmdqueue, &cmd);

    switch(cmd.type) {
      case PLAYBACK_CMD_OPEN:
        if(controller_ready) break;
        log_ddebug("[CONTROL ] EXEC COMMAND: PLAYBACK_CMD_OPEN");

        decoder_t* decoder = (decoder_t*) cmd.arg.ptr;

        uv_mutex_lock(&state.producer_mutex);
        decoder_set(&state.decoder_ops, &decoder->ops);
        state.decoder_data = decoder->data;
        uv_mutex_unlock(&state.producer_mutex);

        uv_mutex_lock(&state.consumer_mutex);
        
        rc = output_device_ops.open(af);
        if (rc < 0) {
          log_error("Unable to open output audio device");
          uv_mutex_unlock(&state.consumer_mutex);
          break;
        }
        output_device_ops.pause();
        uv_mutex_unlock(&state.consumer_mutex);
        
        controller_ready = 1;

        break;
      case PLAYBACK_CMD_CLOSE:
        if(!controller_ready) break;
        log_ddebug("[CONTROL ] EXEC COMMAND: PLAYBACK_CMD_CLOSE");

        uv_mutex_lock(&state.producer_mutex);
        state.producer_ready = 0;
        uv_mutex_unlock(&state.producer_mutex);

        uv_mutex_lock(&state.consumer_mutex);
        state.consumer_ready = 0;
        output_device_ops.drop();
        output_device_ops.close();
        uv_mutex_unlock(&state.consumer_mutex);

        ringbuffer_reset(state.buffer);
        controller_ready = 0;

        break;
      case PLAYBACK_CMD_START:
        if(!controller_ready) break;
        log_ddebug("[CONTROL ] EXEC COMMAND: PLAYBACK_CMD_START");

        uv_mutex_lock(&state.producer_mutex);
        state.producer_ready = 1;
        uv_cond_signal(&state.producer_cond);
        uv_mutex_unlock(&state.producer_mutex);

        uv_mutex_lock(&state.consumer_mutex);
        output_device_ops.unpause();
        state.consumer_ready = 1;
        uv_cond_signal(&state.consumer_cond);
        uv_mutex_unlock(&state.consumer_mutex);

        break;
      case PLAYBACK_CMD_STOP:
        if(!controller_ready) break;
        log_ddebug("[CONTROL ] EXEC COMMAND: PLAYBACK_CMD_STOP");

        uv_mutex_lock(&state.producer_mutex);
        state.producer_ready = 0;
        uv_mutex_unlock(&state.producer_mutex);

        uv_mutex_lock(&state.consumer_mutex);
        state.consumer_ready = 0;
        output_device_ops.pause();
        uv_mutex_unlock(&state.consumer_mutex);

        break;
      case PLAYBACK_CMD_SEEK:
        if(!controller_ready) break;
        log_ddebug("[CONTROL ] EXEC COMMAND: PLAYBACK_CMD_SEEK");

        double offset = cmd.arg.d;

        uv_mutex_lock(&state.producer_mutex);
        uv_mutex_lock(&state.consumer_mutex);

        state.decoder_ops.seek(&state.decoder_data, offset);
        output_device_ops.drop();
        ringbuffer_reset(state.buffer);
        
        uv_mutex_unlock(&state.consumer_mutex);
        uv_mutex_unlock(&state.producer_mutex);

        break;
      case PLAYBACK_CMD_SHUTDOWN:
        if(controller_ready) break;
        log_ddebug("[CONTROL ] EXEC COMMAND: PLAYBACK_CMD_SHUTDOWN");

        uv_mutex_lock(&state.producer_mutex);
        state.producer_enabled = 0;
        uv_mutex_unlock(&state.producer_mutex);

        uv_mutex_lock(&state.consumer_mutex);
        state.consumer_enabled = 0;
        uv_mutex_unlock(&state.consumer_mutex);

        break;
      default:
        break;
    }
  }
}

static void producer_loop(void* arg) {
  log_ddebug("[PRODUCER] STARTING UP");

  uint8_t buf[32 * 1024];
  size_t r;
  size_t w;
  size_t space;

  while(1) {
    uv_mutex_lock(&state.producer_mutex);
    if (!state.producer_enabled) {
      uv_mutex_unlock(&state.producer_mutex);
      break;
    }

    if (!state.producer_ready) {
      uv_cond_wait(&state.producer_cond, &state.producer_mutex);
      uv_mutex_unlock(&state.producer_mutex);
      continue;
    }

    uv_mutex_unlock(&state.producer_mutex);

    space = ringbuffer_get_space(state.buffer);

    if(space <= 0) {
      msleep(50);
      continue;
    }
    
    uv_mutex_lock(&state.producer_mutex);
    r = state.decoder_ops.read(&state.decoder_data, buf, min(sizeof(buf), space));
    uv_mutex_unlock(&state.producer_mutex);

    if(r <= 0) {
      msleep(50);
      continue;
    }
    
    w = ringbuffer_write(state.buffer, buf, r);

    if(w <= 0) {
      //Something gone wrong
      msleep(50); //In any strange situation, go to sleep
    }

    log_dtrace("[PRODUCER] %d bytes written to buffer", w);
  }
}

static void consumer_loop(void* arg) {
  log_ddebug("[CONSUMER] STARTING UP");

  uint8_t buf[32 * 1024];
  size_t r;
  size_t w;
  size_t available;
  size_t space;

  while(1) {
    uv_mutex_lock(&state.consumer_mutex);
    if (!state.consumer_enabled) {
      uv_mutex_unlock(&state.consumer_mutex);
      break;
    };

    if (!state.consumer_ready) {
      uv_cond_wait(&state.consumer_cond, &state.consumer_mutex);
      uv_mutex_unlock(&state.consumer_mutex);
      continue;
    }

    uv_mutex_unlock(&state.consumer_mutex);

    space = output_device_ops.buffer_space();

    if(space <= 0) {
      msleep(10);
      continue;
    }
    
    available = ringbuffer_get_available(state.buffer);

    if(available <= 0) {
      msleep(10);
      continue;
    }

    r = ringbuffer_read(state.buffer, buf, min(sizeof(buf), min(available, space)));

    uv_mutex_lock(&state.consumer_mutex);
    w = output_device_ops.write(buf, r);
    uv_mutex_unlock(&state.consumer_mutex);

    if(w <= 0) {
      msleep(50);
    }
    
    log_dtrace("[CONSUMER] %d bytes written to output device", w);
  }

  uv_mutex_lock(&state.consumer_mutex);
  output_device_ops.close();
  output_device_ops.destroy();
  uv_mutex_unlock(&state.consumer_mutex);
}

void playback_init() {
  state.buffer = ringbuffer_create(512 * 1024);
  state.cmdqueue = command_queue_create(10);
  state.producer_ready = state.consumer_ready = 0;
  state.producer_enabled = state.consumer_enabled = 1;

  int rc;

  rc = output_device_ops.init();
  if (rc < 0) {
    log_error("Unable to initialize output audio device");
    return;
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

  rc = uv_cond_init(&state.producer_cond);
  if (rc < 0) {
    log_fatal("uv_cond_init(producer_cond)");
    exit(EXIT_FAILURE);
  }

  rc = uv_cond_init(&state.consumer_cond);
  if (rc < 0) {
    log_fatal("uv_cond_init(consumer_cond)");
    exit(EXIT_FAILURE);
  }

  rc = uv_thread_create(&state.control_thread, control_loop, NULL);
  if (rc < 0) {
    log_fatal("uv_thread_create(control_thread)");
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

  //rc = uv_thread_join(&state.control_thread);
  //if (rc < 0) {
  //  log_fatal("uv_thread_join(control_thread)");
  //  exit(EXIT_FAILURE);
  //}
}

int playback_ctl(command_t cmd) {
  log_ddebug("[CONTROL ] PUSH COMMAND %d", cmd.type);

  return command_queue_push(state.cmdqueue, cmd);
}

int playback_open(decoder_t* decoder) {
  command_t cmd = { PLAYBACK_CMD_OPEN, {.ptr = decoder} };
  return command_queue_push(state.cmdqueue, cmd);
}

int playback_close() {
  command_t cmd = { PLAYBACK_CMD_CLOSE };
  return command_queue_push(state.cmdqueue, cmd);
}

int playback_start() {
  command_t cmd = { PLAYBACK_CMD_START };
  return command_queue_push(state.cmdqueue, cmd);
}

int playback_stop() {
  command_t cmd = { PLAYBACK_CMD_STOP };
  return command_queue_push(state.cmdqueue, cmd);
}

int playback_seek(double offset) {
  command_t cmd = { PLAYBACK_CMD_SEEK, {.d = offset }};
  return command_queue_push(state.cmdqueue, cmd);
}

int playback_shutdown() {
  command_t cmd = { PLAYBACK_CMD_SHUTDOWN };
  return command_queue_push(state.cmdqueue, cmd);
}