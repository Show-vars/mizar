#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "logging.h"
//#include "playback.h"
//#include "commandqueue.h"
#include "decoder/decoder_impl.h"
#include "pcm.h"
#include "pcm_conv.h"
#include "audio.h"
#include "output/output.h"
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
  log_write_direct("Mizar (version: %s)", VERSION);

  log_info("Server's pid is %lli", uv_os_getpid());

  output_device_ops.init();
  output_device_ops.open(output_af);
  output_device_ops.pause();
  output_device_ops.unpause();

  audio_io_t *audio;
  audio_output_info_t output_info;
  
  output_info.name = "ALSA";
  output_info.af =  af_endian(0) | af_format(SF_FORMAT_S16) | af_rate(44100) | af_channels(2);
  output_info.callback = output_callback;

  audio_open(&audio, &output_info);

  os_sleep_sc(10);
  //playback_init();
  //osc_ctrl_init();

  //decoder_t decoder = {{0, NULL}, decoder_mp3};
  //decoder.ops.open(&decoder.data);  
  
  //playback_open(&decoder);

  
  //osc_ctrl_start();
  
  return 0;
}
