#include <stdio.h>
#include <stdlib.h>

#include "logging.h"
//#include "playback.h"
//#include "commandqueue.h"
#include "decoder/decoder_impl.h"
#include "pcm.h"
#include "pcm_conv.h"
#include "audio_io.h"
#include "audio_ctrl.h"
#include "output/output.h"
#include "util/common.h"
#include "util/time.h"
//#include "osc_ctrl.h"

audio_format_t output_af = af_endian(0) | af_format(SF_FORMAT_S16) | af_rate(44100) | af_channels(2);
uint8_t output_buf[1024 * 2 * 2];

void output_callback(struct audio_data *data, uint32_t frames, void *param) {
  uint32_t r = pcm_float_to_fixed(output_af, output_buf, data->data[0], frames);
  data->frames = output_device_ops.write(output_buf, r);
}

int main() {
  log_init(MIZAR_LOGLEVEL_DEBUG);
  log_info("Mizar (version: %s)", VERSION);
  log_info("Server's pid is %lli", os_getpid());

  output_device_ops.init();
  output_device_ops.open(output_af);

  audio_io_t *audio;
  audio_output_info_t output_info;
  
  output_info.name = "ALSA";
  output_info.af =  af_endian(0) | af_format(SF_FORMAT_S16) | af_rate(44100) | af_channels(2);
  output_info.callback = output_callback;

  log_info("Started");

  audio_io_open(&audio, &output_info);

  audio_ctrl_t *audio_ctrl;
  audio_ctrl_open(&audio_ctrl, audio);

  os_sleep_sc(230);

  audio_ctrl_close(audio_ctrl);
  audio_io_close(audio);
  
  //osc_ctrl_init();
  //osc_ctrl_start();
  
  return 0;
}
