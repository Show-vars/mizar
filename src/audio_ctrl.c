#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include "audio_ctrl.h"
#include "audio_io.h"
#include "logging.h"
#include "util/mem.h"
#include "util/time.h"

#define AUDIO_CTRL_STATE_CLOSED 0
#define AUDIO_CTRL_STATE_OPENED 1

struct audio_ctrl {
  pthread_t thread;

  bool initialized;
  atomic_int state;

  audio_io_t *audio;
  struct audio_io_realtime_data realtime_data;
};

static void *ctrl_thread(void *param) {
  struct audio_ctrl *ctrl = param;

  uint64_t wait_time = 1e9 / 20;
  uint64_t curr_time, last_time = os_gettime_ns();
  uint64_t delta;

  while(ctrl->state == AUDIO_CTRL_STATE_OPENED) {
    audio_io_get_realtime_data(ctrl->audio, &ctrl->realtime_data);

    log_ddebug("[REALTIME] TIME: %lld, PPS: %.1f, PEAK: %f, RMS: %f",
      ctrl->realtime_data.time,
      ctrl->realtime_data.pps,
      ctrl->realtime_data.peak[0][0],
      ctrl->realtime_data.rms[0][0]
    );

    delta = os_gettime_ns() - last_time;    
    os_sleep_ns(wait_time - delta);
    curr_time = os_gettime_ns();
    delta = curr_time - last_time;
    last_time = curr_time;
  }

  return NULL;
}

int audio_ctrl_open(audio_ctrl_t **audio_ctrl, audio_io_t *audio) {
  if(!audio)
    return AUDIO_CTRL_INVALIDPARAM;

  struct audio_ctrl *ctrl;

  if (!(ctrl = zalloc(sizeof(struct audio_ctrl))))
    goto fail;

  ctrl->audio = audio;
  ctrl->state = AUDIO_CTRL_STATE_OPENED;

  if (pthread_create(&ctrl->thread, NULL, ctrl_thread, ctrl) != 0)
    goto fail;

  ctrl->initialized = true;
  *audio_ctrl = ctrl;
  return AUDIO_CTRL_SUCCESS;

  fail:
  return AUDIO_CTRL_ERROR;
}
void audio_ctrl_close(audio_ctrl_t *ctrl) {
  if (!ctrl) return;

  if (ctrl->initialized) {
    ctrl->state = AUDIO_CTRL_STATE_OPENED;
    pthread_join(ctrl->thread, NULL);
  }

  free(ctrl);
}