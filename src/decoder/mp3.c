#include <stdlib.h>
#include <stdint.h>
#include "pcm.h"
#include "util.h"
#include "decoder/decoder_impl.h"

#define DR_MP3_IMPLEMENTATION
#include "decoder/dr_libs/dr_mp3.h"


int decoder_mp3_open(decoder_data_t *data) {
  drmp3 *mp3 = mmalloc(sizeof(drmp3));
  data->af = af_endian(0) | af_format(SF_FORMAT_S16) | af_rate(44100) | af_channels(2);
  data->priv = mp3;

  int r = drmp3_init_file(mp3, "test6.mp3", NULL);

  return r;
}

int decoder_mp3_close(decoder_data_t *data) {
  drmp3 *mp3 = data->priv;

  drmp3_uninit(mp3);
  free(data->priv);

  return 0;
}

size_t decoder_mp3_read_s16(decoder_data_t *data, uint8_t *buffer, size_t frames) {
  drmp3 *mp3 = data->priv;

  size_t mp3_frames = drmp3_read_pcm_frames_s16(mp3, frames, (drmp3_int16*) buffer);

  return mp3_frames;
}

int decoder_mp3_seek(decoder_data_t *data, long offset) {
  drmp3 *mp3 = data->priv;

  int r = drmp3_seek_to_pcm_frame(mp3, af_get_rate(data->af) * offset / 1000);

  return r;
}

long decoder_mp3_duration(decoder_data_t *data) {
  drmp3 *mp3 = data->priv;

  long frames = drmp3_get_pcm_frame_count(mp3) / af_get_rate(data->af);

  return frames;
}

long decoder_mp3_bitrate_current(decoder_data_t *data) {
  drmp3 *mp3 = data->priv;
  return mp3->frameInfo.bitrate_kbps * 1024;
}

long decoder_mp3_bitrate(decoder_data_t *data) {
  return decoder_mp3_bitrate_current(data);
}


static const char *const mp3_ext[]  = { "mp3", "mp2", NULL };
static const char *const mp3_mime[] = { "audio/mpeg", NULL };

const decoder_ops_t decoder_mp3 = {
  .open = decoder_mp3_open,
  .close = decoder_mp3_close,

  .read_s16 = decoder_mp3_read_s16,
  .seek = decoder_mp3_seek,

  .duration = decoder_mp3_duration,
  .bitrate = decoder_mp3_bitrate,
  .bitrate_current = decoder_mp3_bitrate_current
};

const decoder_info_t decoder_mp3_info = {
    .name = "mp3",
    .impl = "dr_mp3",
    .ext = mp3_ext,
    .mime = mp3_mime
  };