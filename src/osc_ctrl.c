#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <uv.h>
#include "osc_ctrl.h"
#include "osc.h"
#include "logging.h"
#include "playback.h"
#include "util/djb2_hash.h"

#define OSC_UDP_PORT 10026

uv_loop_t loop;
uv_udp_t udp_socket;
uv_udp_send_t udp_send_req;

uint8_t rcv_buffer[512];
uint8_t snd_buffer[512];

uv_buf_t msg;

static void memalloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = &rcv_buffer;
    buf->len = sizeof(rcv_buffer);
}

static void on_read(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
  if (nread < 0) {
    log_debug("UDP read error: %s\n", uv_err_name(nread));
    return;
  }

  if (nread == 0) {
    return;
  }

  osc_message_t osc;
  if (!osc_parse_message(&osc, buf->base, nread)) {
    const char* address = osc_get_address(&osc);
    const char* format = osc_get_format(&osc);
    log_debug("Received OSC message (%i bytes): %s %s", nread, address, format);

    switch(djb2_hash(address)) {
      case -1302151709: { // /playback/ctrl
        if(!format[0] || format[0] != 'i') break;

        int state = osc_next_int32(&osc);
        if(state) {
          playback_start();
        } else {
          playback_stop();
        }

        printf("%d\n", state);

        osc_ctrl_send(addr, "/playback/ctrl", "i", state);
        break;
      }
      case 2993374354: { // /playback/seek
        if(!format[0] || format[0] != 'i') break;
        int offset = osc_next_int32(&osc);
        playback_seek(offset);
        break;
      }
    }
  } else {
    log_debug("Unable to parse OSC message: %i bytes", nread);
  }
}

static void on_send(uv_udp_send_t *send_req, int status) {
  free(send_req);
}

struct sockaddr_in send_addr;

void on_playback_realtime_data(playback_realtime_data_t* realtime_data) {
  osc_ctrl_send((const struct sockaddr *)&send_addr, "/playback/time", "i", realtime_data->time);
}

void osc_ctrl_init() {
  uv_loop_init(&loop);

  uv_udp_init(&loop, &udp_socket);
  
  struct sockaddr_in recv_addr;
  uv_ip4_addr("192.168.1.18", 8080, &send_addr);
  uv_ip4_addr("0.0.0.0", OSC_UDP_PORT, &recv_addr);
  uv_udp_bind(&udp_socket, (const struct sockaddr *) &recv_addr, 0);
  uv_udp_recv_start(&udp_socket, memalloc_cb, on_read);

  log_info("OSC UDP server started on port %d", OSC_UDP_PORT);
}

void osc_ctrl_start() {
  uv_run(&loop, UV_RUN_DEFAULT);
}

int osc_ctrl_send(const struct sockaddr *addr, const char *address, const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  const int i = osc_vwrite_message(snd_buffer, sizeof(snd_buffer), address, format, arg);
  va_end(arg);

  msg.base = snd_buffer;
  msg.len = i;

  uv_udp_send_t* send_req = malloc(sizeof(uv_udp_send_t));
  uv_udp_send(send_req, &udp_socket, &msg, 1, addr, on_send);

  return i;
}