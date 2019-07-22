#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "output.h"
#include "pcm.h"
#include "logging.h"
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
                          int err, const char *fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);
  log_writev(MIZAR_LOGLEVEL_WARN, "ALSA", fmt, argptr);
  va_end(argptr);
}

static int alsa_set_hw_params() {
  int rc, dir;

  snd_pcm_hw_params_t *hwparams = NULL;
  rc = snd_pcm_hw_params_malloc(&hwparams);
  if (rc < 0) {
    log_error("snd_pcm_hw_params_malloc");
    return -1;
  }

  rc = snd_pcm_hw_params_any(alsa_handle, hwparams);
  if (rc < 0) {
    log_error("Error: snd_pcm_hw_params_any");
    return -1;
  }

  unsigned int buffer_time_max = ALSA_BUFFER_TIME_MAX;
  rc = snd_pcm_hw_params_set_buffer_time_max(alsa_handle, hwparams, &buffer_time_max, &dir);
  if (rc < 0) {
    log_error("Error: snd_pcm_hw_params_set_buffer_time_max");
    return -1;
  }

  alsa_can_pause = snd_pcm_hw_params_can_pause(hwparams);

  rc = snd_pcm_hw_params_set_access(alsa_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (rc < 0) {
    log_error("Error: snd_pcm_hw_params_set_access");
    return -1;
  }

  alsa_fmt = snd_pcm_build_linear_format(
      af_get_depth(alsa_af), af_get_depth(alsa_af),
      af_get_signed(alsa_af) ? 0 : 1, af_get_endian(alsa_af));
  rc = snd_pcm_hw_params_set_format(alsa_handle, hwparams, alsa_fmt);
  if (rc < 0) {
    log_error("Error: snd_pcm_hw_params_set_format");
    return -1;
  }

  rc = snd_pcm_hw_params_set_channels(alsa_handle, hwparams,
                                      af_get_channels(alsa_af));
  if (rc < 0) {
    log_error("Error: snd_pcm_hw_params_set_channels");
    return -1;
  }

  unsigned int rate = af_get_rate(alsa_af);
  dir = 0;
  rc = snd_pcm_hw_params_set_rate_near(alsa_handle, hwparams, &rate, &dir);
  if (rc < 0) {
    log_error("Error: snd_pcm_hw_params_set_rate_near");
    return -1;
  }

  rc = snd_pcm_hw_params(alsa_handle, hwparams);
  if (rc < 0) {
    log_error("Error:  snd_pcm_hw_params");
    return -1;
  }

  snd_pcm_hw_params_free(hwparams);
  return rc;
}

static int output_alsa_init() {
  int rc;

  snd_lib_error_set_handler(error_handler);

  rc = snd_pcm_status_malloc(&status);
  if (rc < 0) {
    log_error("Error: snd_pcm_status_malloc");
    return -1;
  }

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

  rc = snd_pcm_open(&alsa_handle, alsa_device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (rc < 0) {
    log_error("Error: snd_pcm_open");
    return -1;
  }

  rc = alsa_set_hw_params();
  if (rc < 0) {
    log_error("Error: alsa_set_hw_params");
    return -1;
  }

  rc = snd_pcm_prepare(alsa_handle);
  if (rc < 0) {
    log_error("Error: snd_pcm_prepare");
    return -1;
  }

  rc = snd_pcm_nonblock(alsa_handle, 1);
  if (rc < 0) {
    log_error("Error: snd_pcm_nonblock");
    return -1;
  }

  return 0;
}

static int output_alsa_close() {
  int rc;

  rc = snd_pcm_drain(alsa_handle);
  log_ddebug("snd_pcm_drain: %d", rc);

  rc = snd_pcm_close(alsa_handle);
  log_ddebug("snd_pcm_close: %d", rc);

  return rc ? -1 : 0;
}

static int output_alsa_drop() {
	int rc;

	rc = snd_pcm_drop(alsa_handle);
  log_ddebug("snd_pcm_drop: %d", rc);

	rc = snd_pcm_prepare(alsa_handle);
  log_ddebug("snd_pcm_prepare: %d", rc);

	return rc ? -1 : 0;
}

static size_t output_alsa_write(const uint8_t *buf, size_t frames) {
  snd_pcm_sframes_t alsa_frames;

  alsa_frames = snd_pcm_writei(alsa_handle, buf, frames);
  if (alsa_frames < 0) alsa_frames = snd_pcm_recover(alsa_handle, alsa_frames, 0);
  if (alsa_frames < 0) {
    log_ddebug("snd_pcm_writei failed: %s", snd_strerror(alsa_frames));
    return 0;
  }

  return alsa_frames;
}

static size_t output_alsa_buffer_space() {
  snd_pcm_sframes_t alsa_frames;

  alsa_frames = snd_pcm_avail_update(alsa_handle);
  if (alsa_frames < 0) alsa_frames = snd_pcm_recover(alsa_handle, alsa_frames, 0);
  if (alsa_frames < 0) {
    log_ddebug("snd_pcm_avail_update failed: %s\n", snd_strerror(alsa_frames));
    return 0;
  }

  return alsa_frames;
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
        log_ddebug("error: state is not RUNNING or PREPARED");
        break;
    }

    return 0;
  } else {
    log_ddebug("snd_pcm_drop");
    return snd_pcm_drop(alsa_handle);
  }
}

static int output_alsa_unpause() {
  if (alsa_can_pause) {
    snd_pcm_state_t state = snd_pcm_state(alsa_handle);

    switch (state) {
      case SND_PCM_STATE_PREPARED:
        break;
      case SND_PCM_STATE_PAUSED:
        snd_pcm_wait(alsa_handle, -1);
        snd_pcm_pause(alsa_handle, 0);
        break;
      default:
        log_ddebug("error: state is not PAUSED or PREPARED");
        break;
    }

    return 0;
  } else {
    log_ddebug("snd_pcm_prepare");
    return snd_pcm_prepare(alsa_handle);
  }
}

const struct output_device output_device_ops = {
    .init = output_alsa_init,
    .destroy = output_alsa_destroy,
    .open = output_alsa_open,
    .close = output_alsa_close,
    .drop = output_alsa_drop,
    .write = output_alsa_write,
    .buffer_space = output_alsa_buffer_space,
    .pause = output_alsa_pause,
    .unpause = output_alsa_unpause,
};