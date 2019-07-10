#include <alsa/asoundlib.h>
#include <stdint.h>
#include "output.h"
#include "pcm.h"
#include "util.h"

#define ALSA_BUFFER_TIME_MAX 300 * 1000;  // 300 ms

static audio_format_t alsa_af;

static char *alsa_device = "default";
static snd_pcm_t *alsa_handle;
static snd_pcm_format_t alsa_fmt;
static int alsa_frame_size;
static int alsa_can_pause;
static snd_pcm_status_t *status;

/* dummy alsa error handler */
static void error_handler(const char *file, int line, const char *function,
                          int err, const char *fmt, ...) {}

static int alsa_set_hw_params() {
  int rc, dir;

  snd_pcm_hw_params_t *hwparams = NULL;
  rc = snd_pcm_hw_params_malloc(&hwparams);
  if (rc < 0) mdie("Error: snd_pcm_hw_params_malloc");

  rc = snd_pcm_hw_params_any(alsa_handle, hwparams);
  if (rc < 0) mdie("Error: snd_pcm_hw_params_any");

  unsigned int buffer_time_max = ALSA_BUFFER_TIME_MAX;
  rc = snd_pcm_hw_params_set_buffer_time_max(alsa_handle, hwparams,
                                             &buffer_time_max, &dir);
  if (rc < 0) mdie("Error: snd_pcm_hw_params_set_buffer_time_max");

  alsa_can_pause = snd_pcm_hw_params_can_pause(hwparams);

  rc = snd_pcm_hw_params_set_access(alsa_handle, hwparams,
                                    SND_PCM_ACCESS_RW_INTERLEAVED);
  if (rc < 0) mdie("Error: snd_pcm_hw_params_set_access");

  alsa_fmt = snd_pcm_build_linear_format(
      af_get_depth(alsa_af), af_get_depth(alsa_af),
      af_get_signed(alsa_af) ? 0 : 1, af_get_endian(alsa_af));
  rc = snd_pcm_hw_params_set_format(alsa_handle, hwparams, alsa_fmt);
  if (rc < 0) mdie("Error: snd_pcm_hw_params_set_format");

  rc = snd_pcm_hw_params_set_channels(alsa_handle, hwparams,
                                      af_get_channels(alsa_af));
  if (rc < 0) mdie("Error: snd_pcm_hw_params_set_channels");

  unsigned int rate = af_get_rate(alsa_af);
  dir = 0;
  rc = snd_pcm_hw_params_set_rate_near(alsa_handle, hwparams, &rate, &dir);
  if (rc < 0) mdie("Error: snd_pcm_hw_params_set_rate_near");

  rc = snd_pcm_hw_params(alsa_handle, hwparams);
  if (rc < 0) mdie("Error: snd_pcm_hw_params");

  snd_pcm_hw_params_free(hwparams);
  return rc;
}

static int output_alsa_init() {
  int rc;

  snd_lib_error_set_handler(error_handler);

  rc = snd_pcm_status_malloc(&status);

  return rc;
}

static int output_alsa_destroy() {
  snd_pcm_status_free(status);

  return 0;
}

static int output_alsa_open(audio_format_t af) {
  int rc;

  alsa_af = af;
  alsa_frame_size = af_get_frame_size(af);

  rc = snd_pcm_open(&alsa_handle, alsa_device, SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) mdie("Error: snd_pcm_open");

  rc = alsa_set_hw_params();
  if (rc < 0) mdie("Error: alsa_set_hw_params");

  rc = snd_pcm_prepare(alsa_handle);
  if (rc < 0) mdie("Error: snd_pcm_prepare");

  return 0;
}

static int output_alsa_close() {
  int rc;

  rc = snd_pcm_drain(alsa_handle);
  mdebug("snd_pcm_drain: %d", rc);

  rc = snd_pcm_close(alsa_handle);
  mdebug("snd_pcm_close: %d", rc);

  return rc;
}

static int output_alsa_write(const uint8_t *buf, size_t len) {
  snd_pcm_sframes_t frames;
  int flen = len / alsa_frame_size;

  frames = snd_pcm_writei(alsa_handle, buf, flen);
  if (frames < 0) frames = snd_pcm_recover(alsa_handle, frames, 0);
  if (frames < 0) {
    mdebug("snd_pcm_writei failed: %s\n", snd_strerror(frames));
    return -1;
  }

  return frames * alsa_frame_size;
}

static int output_alsa_buffer_space() {
  snd_pcm_sframes_t frames;

  frames = snd_pcm_avail_update(alsa_handle);
  if (frames < 0) frames = snd_pcm_recover(alsa_handle, frames, 0);
  if (frames < 0) {
    mdebug("snd_pcm_avail_update failed: %s\n", snd_strerror(frames));
    return 0;
  }

  return frames * alsa_frame_size;
}

static int output_alsa_pause() {
  if (alsa_can_pause) {
    snd_pcm_state_t state = snd_pcm_state(alsa_handle);

    switch (state) {
      case SND_PCM_STATE_PREPARED:
        break;
      case SND_PCM_STATE_RUNNING:
        snd_pcm_wait(alsa_handle, -1);
        snd_pcm_pause(alsa_handle, 1);
        break;
      default:
        mdebug("error: state is not RUNNING or PREPARED");
        break;
    }

    return 0;
  } else {
    mdebug("snd_pcm_drop");
    return snd_pcm_drop(alsa_handle);
  }
}

static int output_alsa_unpause() {
  if (alsa_can_pause) {
    snd_pcm_state_t state = snd_pcm_state(alsa_handle);

    switch (state) {
      case SND_PCM_STATE_PREPARED:
        break;
      case SND_PCM_STATE_RUNNING:
        snd_pcm_wait(alsa_handle, -1);
        snd_pcm_pause(alsa_handle, 0);
        break;
      default:
        mdebug("error: state is not PAUSED or PREPARED");
        break;
    }

    return 0;
  } else {
    mdebug("snd_pcm_prepare");
    return snd_pcm_prepare(alsa_handle);
  }
}

const struct output_device output_device_ops = {
    .init = output_alsa_init,
    .destroy = output_alsa_destroy,
    .open = output_alsa_open,
    .close = output_alsa_close,
    .write = output_alsa_write,
    .buffer_space = output_alsa_buffer_space,
    .pause = output_alsa_pause,
    .unpause = output_alsa_unpause,
};