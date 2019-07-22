#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <uv.h>
#include "playback.h"
#include "audiobuffer.h"
#include "commandqueue.h"
#include "extend.h"
#include "output/output.h"
#include "decoder/decoder.h"
#include "pcm.h"
#include "pcm_conv.h"
#include "logging.h"
#include "util.h"

#define PLAYBACK_BUFFER_FRAMES 1024 * 8
#define PLAYBACK_IO_BUFFER_SAMPLES 1024
#define PLAYBACK_PPS_MAX_SAMPLES 10
#define PLAYBACK_REALTIME_PUSH_FACTOR 1

struct playback_state {
  audiobuffer_t* buffer;
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
audio_format_t internal_af = af_endian(0) | af_format(SF_FORMAT_FLOAT) | af_rate(44100) | af_channels(2);
audio_format_t output_af = af_endian(0) | af_format(SF_FORMAT_S16) | af_rate(44100) | af_channels(2);

int tickindex = 0;
double ticksum = 0;
uint64_t ticklist[PLAYBACK_PPS_MAX_SAMPLES];
int realtime_push_count = 0;

playback_realtime_data_t playback_realtime_data;

static void playback_calculate_time(uint64_t frames) {
  playback_realtime_data.time = pcm_frames_to_ns(af_get_rate(internal_af), frames) / 1e9;
}

static void playback_calculate_pps(uint64_t delta) {
  uint64_t newtick = 1e9 / delta;

  ticksum = ticksum - ticklist[tickindex] + newtick;
  ticklist[tickindex] = newtick;
  tickindex = (tickindex + 1) % PLAYBACK_PPS_MAX_SAMPLES;

  playback_realtime_data.pps = (ticksum / PLAYBACK_PPS_MAX_SAMPLES);
}

float peak_last[MAX_AUDIO_CHANNELS][4];

static void playback_calculate_peak(float* buffer, size_t frames) {
  int channels = af_get_channels(internal_af);

  for(int ch = 0; ch < channels && ch < MAX_AUDIO_CHANNELS; ch++) {
    float peak = max(peak_last[ch][0], max(peak_last[ch][1], max(peak_last[ch][2], peak_last[ch][3])));
    for(size_t i = 0; i < frames; i++) {
      float sample = buffer[i * channels + ch];
      peak = fmaxf(peak, fabsf(sample));
    }

    peak_last[ch][0] = buffer[(frames-4) * channels + ch];
    peak_last[ch][1] = buffer[(frames-3) * channels + ch];
    peak_last[ch][2] = buffer[(frames-2) * channels + ch];
    peak_last[ch][3] = buffer[(frames-1) * channels + ch];
    
    playback_realtime_data.peak[ch] = pcm_to_db(peak);
  }
}

static void playback_calculate_rms(float* buffer, size_t frames) {
  int channels = af_get_channels(internal_af);

  for(int ch = 0; ch < channels; ch++) {
    float sum = 0.0;

    for(size_t i = 0; i < frames; i++) {
      float sample = buffer[i * channels + ch];
      sum += sample * sample;
    }

    playback_realtime_data.rms[ch] = pcm_to_db(sqrtf(sum / frames));
  }
}

static void playback_push_realtime_data(playback_realtime_data_t* realtime_data) {
  log_ddebug("[REALTIME] TIME: %lld, PPS: %d, PEAK: %f, RMS: %f", realtime_data->time, realtime_data->pps, realtime_data->peak[0],  realtime_data->rms[0]);
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
        
        rc = output_device_ops.open(output_af);
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

        long offset = cmd.arg.l;

        uv_mutex_lock(&state.producer_mutex);
        uv_mutex_lock(&state.consumer_mutex);

        state.decoder_ops.seek(&state.decoder_data, offset);
        output_device_ops.drop();
        audiobuffer_reset(state.buffer);
        audiobuffer_set_frames(state.buffer, pcm_ns_to_frames(af_get_rate(internal_af), (uint64_t) offset * 1e6));
        
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

  float* ptr;
  uint8_t input_buf[PLAYBACK_IO_BUFFER_SAMPLES];
  size_t r, w, space;

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

    space = audiobuffer_write_begin(state.buffer, PLAYBACK_BUFFER_FRAMES);
    if(space > 0) {
      uv_mutex_lock(&state.producer_mutex);
      while((r = audiobuffer_write(state.buffer, &ptr)) > 0) {
        r = state.decoder_ops.read_s16(&state.decoder_data, input_buf, min(r, PLAYBACK_IO_BUFFER_SAMPLES / af_get_frame_size(state.decoder_data.af)));
        r = pcm_fixed_to_float(state.decoder_data.af, ptr, input_buf, r);
        w = audiobuffer_write_fill(state.buffer, r);
      }
      uv_mutex_unlock(&state.producer_mutex);
    }
    w = audiobuffer_write_end(state.buffer);

    if(w > 0) {
      log_dtrace("[PRODUCER] %d frames written to buffer", w);
    }

    ms_sleep(20);
  }
}

static void consumer_loop(void* arg) {
  log_ddebug("[CONSUMER] STARTING UP");

  float *ptr;
  uint8_t output_buf[PLAYBACK_IO_BUFFER_SAMPLES];
  size_t r, w, available, space;

  uint64_t audio_wait_time = pcm_frames_to_ns(af_get_rate(internal_af), PLAYBACK_BUFFER_FRAMES / 2);
  uint64_t delta = 1;
  
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

    if(space <= 0) {
      goto consumer_end_sleep;
    }
    
    available = audiobuffer_read_begin(state.buffer, space);
    if(available > 0) {
      uv_mutex_lock(&state.consumer_mutex);
      while((r = audiobuffer_read(state.buffer, &ptr)) > 0) {
        r = pcm_float_to_fixed(output_af, output_buf, ptr, min(r, PLAYBACK_IO_BUFFER_SAMPLES / af_get_frame_size(output_af)));

        playback_calculate_peak(ptr, r);
        playback_calculate_rms(ptr, r);

        r = output_device_ops.write(output_buf, r);
        w = audiobuffer_read_consume(state.buffer, r);
      }
      uv_mutex_unlock(&state.consumer_mutex);
    }
    w = audiobuffer_read_end(state.buffer);

    if(w > 0) {
      log_dtrace("[CONSUMER] %d frames written to the output device", w);
    }
    
    consumer_end_sleep:

    delta = uv_hrtime() - last_time;    
    ns_sleep(audio_wait_time - delta);
    delta = uv_hrtime() - last_time;  
    last_time = uv_hrtime();

    playback_calculate_pps(delta);
    playback_calculate_time(audiobuffer_get_frames(state.buffer));

    if(++realtime_push_count >= PLAYBACK_REALTIME_PUSH_FACTOR) {
      realtime_push_count = 0;
      playback_push_realtime_data(&playback_realtime_data);
    }
  }

  uv_mutex_lock(&state.consumer_mutex);
  output_device_ops.close();
  output_device_ops.destroy();
  uv_mutex_unlock(&state.consumer_mutex);
}

void playback_init() {
  state.buffer = audiobuffer_create(output_af, PLAYBACK_BUFFER_FRAMES);
  state.cmdqueue = command_queue_create(8);
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

int playback_seek(long offset) {
  command_t cmd = { PLAYBACK_CMD_SEEK, {.l = offset }};
  return command_queue_push(state.cmdqueue, cmd);
}

int playback_shutdown() {
  command_t cmd = { PLAYBACK_CMD_SHUTDOWN };
  return command_queue_push(state.cmdqueue, cmd);
}