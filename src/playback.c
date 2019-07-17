#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <uv.h>
#include <inttypes.h>
#include "playback.h"
#include "audiobuffer.h"
#include "commandqueue.h"
#include "extend.h"
#include "output/output.h"
#include "decoder/decoder.h"
#include "log.h"
#include "util.h"

#define PLAYBACK_OUTPUT_FRAMES 1024

struct audio_data {
  float data[PLAYBACK_OUTPUT_FRAMES][MAX_AUDIO_CHANNELS];
};

struct playback_state {
  audiobuffer_t* buffer;
  command_queue_t* cmdqueue;

  decoder_ops_t decoder_ops;
  decoder_data_t decoder_data;

  int producer_enabled;
  int consumer_enabled;

  int producer_ready;
  int consumer_ready;
  
  uv_mutex_t producer_mutex;
  uv_mutex_t consumer_mutex;

  uv_cond_t producer_cond;
  uv_cond_t consumer_cond;

  uv_thread_t control_thread;
  uv_thread_t producer_thread;
  uv_thread_t consumer_thread;
};

struct playback_state state;
playback_realtime_data_t playback_realtime_data;

audio_format_t af = af_endian(0) | af_signed(1) | af_depth(16) |
                      af_rate(44100) | af_channels(2);

static void playback_push_realtime_data(playback_realtime_data_t* realtime_data) {

}

static void playback_calculate_rms_peak(double* rms, double* peak) {
  

}

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

        audiobuffer_reset(state.buffer);
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
        audiobuffer_reset(state.buffer);
        audiobuffer_set_time(state.buffer, offset);
        
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

  float buffer[MAX_AUDIO_CHANNELS * PLAYBACK_OUTPUT_FRAMES];
  audio_data_t audio;
  size_t r, w, space;

  audio.channels = af_get_channels(af);
  audio.sample_rate = af_get_rate(af);
  audio.frames = PLAYBACK_OUTPUT_FRAMES;
  audio.data = buffer;

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

    space = audiobuffer_get_space(state.buffer);

    if(space <= 0) {
      ms_sleep(50);
      continue;
    }
    
    uv_mutex_lock(&state.producer_mutex);
    r = state.decoder_ops.read(&state.decoder_data, audio.data, min(space, PLAYBACK_OUTPUT_FRAMES));
    uv_mutex_unlock(&state.producer_mutex);

    if(r <= 0) {
      ms_sleep(50);
      continue;
    }
    
    w = audiobuffer_write(state.buffer, audio.data, r);

    if(w <= 0) {
      ms_sleep(50);
      continue;
    }

    audiobuffer_fill(state.buffer, w);

    log_dtrace("[PRODUCER] %d bytes written to buffer", w);
  }
}

static void consumer_loop(void* arg) {
  log_ddebug("[CONSUMER] STARTING UP");

  float buffer[MAX_AUDIO_CHANNELS * PLAYBACK_OUTPUT_FRAMES];
  uint8_t output_buffer[MAX_AUDIO_CHANNELS * PLAYBACK_OUTPUT_FRAMES];
  
  audio_data_t audio;
  audio_io_data_t output_audio;
  size_t r, w, available, space;

  audio.channels = af_get_channels(af);
  audio.sample_rate = af_get_rate(af);
  audio.frames = PLAYBACK_OUTPUT_FRAMES;
  audio.data = buffer;

  output_audio.af = af;
  output_audio.frames = PLAYBACK_OUTPUT_FRAMES;
  output_audio.size = PLAYBACK_OUTPUT_FRAMES * af_get_frame_size(output_audio.af);
  output_audio.data = output_buffer;


  int audio_wait_time = (PLAYBACK_OUTPUT_FRAMES * 1e3 / 44100) * 1e6;
  int delta;
  
  uint64_t last_time = uv_hrtime();
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

    if(space < af_get_frame_size(af) * PLAYBACK_OUTPUT_FRAMES) {
      goto consumer_end_sleep;
    }
    
    available = audiobuffer_get_available(state.buffer);

    if(available < audio.frames) {
      goto consumer_end_sleep;
    }

    
    r = audiobuffer_read(state.buffer, audio.data, audio.frames);

    pcm_fti(output_audio, audio);

    uv_mutex_lock(&state.consumer_mutex);
    w = output_device_ops.write(output_audio.data, output_audio.frames);
    uv_mutex_unlock(&state.consumer_mutex);

    if(w > 0) {
      audiobuffer_consume(state.buffer, output_audio.frames);
      //log_dtrace("[CONSUMER] %d bytes written to output device", w);
    }

    playback_realtime_data.time = audiobuffer_get_time(state.buffer);
    playback_push_realtime_data(&playback_realtime_data);

    consumer_end_sleep:    
    
    delta = uv_hrtime() - last_time;
    ns_sleep(audio_wait_time - delta);

    last_time = uv_hrtime();
  }

  uv_mutex_lock(&state.consumer_mutex);
  output_device_ops.close();
  output_device_ops.destroy();
  uv_mutex_unlock(&state.consumer_mutex);
}

void playback_init() {
  state.buffer = audiobuffer_create(af, PLAYBACK_OUTPUT_FRAMES * 4);
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