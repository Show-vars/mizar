#ifndef _H_OSC_
#define _H_OSC_

#include "playback.h"

void osc_ctrl_init();
void osc_ctrl_start();
int osc_ctrl_send(const struct sockaddr *addr, const char *address, const char *format, ...);
void on_playback_realtime_data(playback_realtime_data_t* realtime_data);


#endif