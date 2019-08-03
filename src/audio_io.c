#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <string.h>
#include "audio_io.h"
#include "pcm.h"
#include "pcm_conv.h"
#include "decoder/decoder_impl.h"
#include "logging.h"
#include "util/mem.h"
#include "util/math.h"
#include "util/time.h"

#define AUDIO_IO_STATE_CLOSED 0
#define AUDIO_IO_STATE_OPENED 1

struct audio_volmeter {
  float peak_last[AUDIO_IO_MAX_CHANNELS][4];
};

struct audio_pps_history {
  int tickindex;
  double ticksum;
  uint64_t ticklist[AUDIO_PPS_SAMPLES];
};

struct audio_bus {
  struct audio_volmeter volmeter;
  float buffer[AUDIO_IO_MAX_CHANNELS][AUDIO_IO_OUTPUT_FRAMES];
};

struct audio_input {
  audio_input_callback_t callback;
  int bus_idx;
  void *param;
  float buffer[AUDIO_IO_MAX_CHANNELS][AUDIO_IO_OUTPUT_FRAMES];
};

struct audio_output {
  audio_output_callback_t callback;
  void *param;
};

struct audio_io {
  pthread_t thread;

  bool initialized;
  atomic_int state;

  uint8_t channels;
  uint32_t framerate;

  struct audio_input  input[AUDIO_IO_INPUTS];
  struct audio_bus    buses[AUDIO_IO_BUSES];
  struct audio_output output;

  struct audio_pps_history pps_history;

  struct audio_io_realtime_data internal_rt_data;
  atomic_bool external_rt_data_ready;
  _Atomic	struct audio_io_realtime_data external_rt_data;
};

//
// Realtime data calculation functions
//
static void audio_calculate_peak(struct audio_io *audio, uint32_t bus_idx, struct audio_data *data) {
  struct audio_volmeter *volmeter = &audio->buses[bus_idx].volmeter;

  for(int ch = 0; ch < AUDIO_IO_MAX_CHANNELS; ch++) {
    float peak = max4(volmeter->peak_last[ch][0], volmeter->peak_last[ch][1], volmeter->peak_last[ch][2], volmeter->peak_last[ch][3]);
    for(uint32_t i = 0; i < data->frames; i++) {
      float sample = data->data[ch][i];
      peak = fmaxf(peak, fabsf(sample));
    }

    audio->internal_rt_data.peak[bus_idx][ch] = pcm_to_db(peak);
  }
}

static void audio_calculate_rms(struct audio_io *audio, uint32_t bus_idx, struct audio_data *data) {
  for(int ch = 0; ch < AUDIO_IO_MAX_CHANNELS; ch++) {
    float sum = 0.0;
    for(uint32_t i = 0; i < data->frames; i++) {
      float sample = data->data[ch][i];
      sum += sample * sample;
    }

    float rms = sqrtf(sum / data->frames);
    audio->internal_rt_data.rms[bus_idx][ch] = pcm_to_db(rms);
  }
}

static void audio_calculate_pps(struct audio_io *audio, uint64_t delta) {
  struct audio_pps_history *ph = &audio->pps_history;

  uint64_t newtick = delta;

  ph->ticksum = ph->ticksum - ph->ticklist[ph->tickindex] + newtick;
  ph->ticklist[ph->tickindex] = newtick;
  ph->tickindex = (ph->tickindex + 1) % AUDIO_PPS_SAMPLES;

  audio->internal_rt_data.pps = (float) 1e9 / (ph->ticksum / AUDIO_PPS_SAMPLES);
}

//
// Audio thread functions
//
decoder_t decoder;
uint8_t input_buf[AUDIO_IO_OUTPUT_FRAMES * 2 * 2];

static void audio_input(struct audio_data *data, uint32_t frames, void *param) {
  uint32_t r = decoder.ops.read_s16(&decoder.data, input_buf, AUDIO_IO_OUTPUT_FRAMES);
  data->frames = pcm_fixed_to_float(decoder.data.af, data->data[0], input_buf, r);
}

static inline void clamp_audio(struct audio_data *mix) {
  size_t frames = mix->frames;

  for (size_t ch = 0; ch < AUDIO_IO_MAX_CHANNELS; ch++) {
    register float *m = mix->data[ch];
    register float *end = &m[frames];

    while (m < end) {
      register float v = *m;
      *(m++) = clamp(v, -1.0f, 1.0f);
    }
  }
}

static inline void mix_audio(struct audio_data *mix, struct audio_data *input) {
  size_t frames = min(mix->frames, input->frames);
  
  for (size_t ch = 0; ch < AUDIO_IO_MAX_CHANNELS; ch++) {
    register float *m = mix->data[ch];
    register float *i = input->data[ch];
    register float *end = i + frames;

    while (i < end) *(m++) += *(i++);
  }
}

static void audio_input_output(struct audio_io *audio) {
  struct audio_data input_data[AUDIO_IO_INPUTS];
  struct audio_data bus_data[AUDIO_IO_INPUTS];

  memset(input_data, 0, sizeof(input_data));
  memset(bus_data, 0, sizeof(bus_data));

  for(int inp_idx = 0; inp_idx < AUDIO_IO_INPUTS; inp_idx++) {
    struct audio_input *input = &audio->input[inp_idx];
    
    memset(input->buffer[0], 0, AUDIO_IO_OUTPUT_FRAMES * AUDIO_IO_MAX_CHANNELS * sizeof(float));

    for(uint8_t ch = 0; ch < AUDIO_IO_MAX_CHANNELS; ch++) {
      input_data[inp_idx].frames = AUDIO_IO_OUTPUT_FRAMES;
      input_data[inp_idx].data[ch] = input->buffer[ch];
    }
  }

  for(int bus_idx = 0; bus_idx < AUDIO_IO_BUSES; bus_idx++) {
    struct audio_bus *bus = &audio->buses[bus_idx];
    
    memset(bus->buffer[0], 0, AUDIO_IO_OUTPUT_FRAMES * AUDIO_IO_MAX_CHANNELS * sizeof(float));

    for(uint8_t ch = 0; ch < AUDIO_IO_MAX_CHANNELS; ch++) {
      bus_data[bus_idx].frames = AUDIO_IO_OUTPUT_FRAMES;
      bus_data[bus_idx].data[ch] = bus->buffer[ch];
    }
  }

  for(int inp_idx = 0; inp_idx < AUDIO_IO_INPUTS; inp_idx++) {
    struct audio_input *input = &audio->input[inp_idx];

    if(input->callback) {
      input->callback(&input_data[inp_idx], AUDIO_IO_OUTPUT_FRAMES, input->param);
      mix_audio(&bus_data[input->bus_idx], &input_data[inp_idx]);
    }
  }

  for(int bus_idx = 0; bus_idx < AUDIO_IO_BUSES; bus_idx++) {
    clamp_audio(&bus_data[bus_idx]);

    audio_calculate_peak(audio, bus_idx, &bus_data[bus_idx]);
    audio_calculate_rms(audio, bus_idx, &bus_data[bus_idx]);
  }

  audio->output.callback(&bus_data[0], AUDIO_IO_OUTPUT_FRAMES, audio->output.param);
}

static void *audio_thread(void *param) {
  struct audio_io *audio = param;

  decoder.ops = decoder_mp3;
  decoder.data.af = 0;
  decoder.data.priv = 0;
  decoder.ops.open(&decoder.data);

  uint64_t curr_time, last_time = os_gettime_ns();
  uint64_t delta;

  bool rt_data_ready_expected = false;

  while(audio->state == AUDIO_IO_STATE_OPENED) {
    audio_input_output(audio);

    curr_time = os_gettime_ns();
    delta = curr_time - last_time;  
    last_time = curr_time;

    audio_calculate_pps(audio, delta);

    // Push new realtime data for external access if not ready;
    if(atomic_compare_exchange_strong(&audio->external_rt_data_ready, &rt_data_ready_expected, true)) {
      audio->external_rt_data = audio->internal_rt_data;
    }
  }

  return NULL;
}

//
// Public functions
//

int audio_io_open(audio_io_t **audio, audio_output_info_t* output_info) {
  if(af_get_channels(output_info->af) > AUDIO_IO_MAX_CHANNELS)
    return AUDIO_IO_INVALIDPARAM;
  
  struct audio_io *io;

  if (!(io = zalloc(sizeof(struct audio_io))))
    goto fail;
  
  io->channels = af_get_channels(output_info->af);
  io->framerate = af_get_rate(output_info->af);

  io->output.callback = output_info->callback;
  io->output.param = output_info->param;
  
  io->state = AUDIO_IO_STATE_OPENED;

  if (pthread_create(&io->thread, NULL, audio_thread, io) != 0)
    goto fail;

  io->input[0].callback = audio_input;
  io->input[0].bus_idx = 0;
  io->initialized = true;
  *audio = io;

  float buffer_time = (float) AUDIO_IO_OUTPUT_FRAMES / af_get_rate(output_info->af);

  log_write(MIZAR_LOGLEVEL_DEBUG, "AUDIO IO", "Buffer time: %.2f ms", buffer_time * 1e3);

  return AUDIO_IO_SUCCESS;

  fail:
  audio_io_close(io);
  return AUDIO_IO_ERROR;
}

void audio_io_close(audio_io_t *audio) {
  if (!audio) return;

  if (audio->initialized) {
    audio->state = AUDIO_IO_STATE_CLOSED;
    pthread_join(audio->thread, NULL);
  }

  free(audio);
}

void audio_io_get_realtime_data(audio_io_t *audio, struct audio_io_realtime_data *realtime_data) {
  audio->external_rt_data_ready = false;
  *realtime_data = audio->external_rt_data;
}