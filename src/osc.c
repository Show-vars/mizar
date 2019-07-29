#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "osc.h"

static inline void osc_encode_uint32(uint32_t val, char *data) {
  uint8_t *v = (uint8_t *)&val;
  data[3] = *(v);
  data[2] = *(v + 1);
  data[1] = *(v + 2);
  data[0] = *(v + 3);
}

static inline uint32_t osc_decode_uint32(char *data) {
  return ((uint32_t)data[3] << 0) | ((uint32_t)data[2] << 8) | ((uint32_t)data[1] << 16) | ((uint32_t)data[0] << 24);
}

static inline void osc_encode_uint64(uint64_t val, char *data) {
  uint8_t *v = (uint8_t *)&val;
  data[7] = *v;
  data[6] = *(v + 1);
  data[5] = *(v + 2);
  data[4] = *(v + 3);
  data[3] = *(v + 4);
  data[2] = *(v + 5);
  data[1] = *(v + 6);
  data[0] = *(v + 7);
}

static inline uint64_t decode_uint64(char *data) {
  return ((uint64_t)data[7] << 0) | ((uint64_t)data[6] << 8) | ((uint64_t)data[5] << 16) | ((uint64_t)data[4] << 24) |
         ((uint64_t)data[3] << 32) | ((uint64_t)data[2] << 40) | ((uint64_t)data[1] << 48) | ((uint64_t)data[0] << 56);
}

int osc_parse_message(osc_message_t* msg, char* buffer, uint32_t len) {
  int i = 0;
  while (buffer[i] != '\0') ++i;
  while (buffer[i] != ',') ++i;
  if (i >= len) return -1; // error: no format string

  msg->format = buffer + i + 1;

  while (i < len && buffer[i] != '\0') ++i;
  if (i == len) return -2; // error: format string in not null terminated

  i = (i + 4) & ~0x3;
  msg->head = buffer + i;

  msg->buffer = buffer;
  msg->size = len;

  return 0;
}

char* osc_get_address(osc_message_t* msg) {
  return msg->buffer; // wow, so simple
}

char* osc_get_format(osc_message_t* msg) {
  return msg->format;
}

uint32_t osc_get_size(osc_message_t* msg) {
  return msg->size;
}

int32_t osc_next_int32(osc_message_t* msg) {
  const int32_t i = osc_decode_uint32(msg->head);
  msg->head += 4;
  return i;
}

float osc_next_float(osc_message_t* msg) {
  const int32_t i = osc_decode_uint32(msg->head);
  msg->head += 4;
  return *((float *) (&i));
}

const char* osc_next_string(osc_message_t* msg) {
  int i = (int) strlen(msg->head);
  if (msg->head + i >= msg->buffer + msg->size) return NULL;
  const char *s = msg->head;
  i = (i + 4) & ~0x3;
  msg->head += i;
  return s;
}

void osc_next_blob(osc_message_t* msg, const char **buffer, int *len) {
  int32_t i = osc_decode_uint32(msg->head);
  if (msg->head + 4 + i <= msg->buffer + msg->size) {
    *len = i;
    *buffer = msg->head + 4;
    i = (i + 7) & ~0x3;
    msg->head += i;
  } else {
    *len = 0;
    *buffer = NULL;
  }
}

int osc_vwrite_message(char *buffer, const int len, const char *address, const char *format, va_list arg) {
  memset(buffer, 0, len);

  int i = (int) strlen(address);
  if (address == NULL || i >= len) return -1;
  strncpy(buffer, address, len);
  i = (i + 4) & ~0x3;
  buffer[i++] = ',';
  int s_len = (int) strlen(format);
  if (format == NULL || (i + s_len) >= len) return -2;
  strncpy(buffer+i, format, len-i-s_len);
  i = (i + 4 + s_len) & ~0x3;

  for (int j = 0; format[j] != '\0'; ++j) {
    switch (format[j]) {
      case 'b': {
        const uint32_t n = (uint32_t) va_arg(arg, int);
        if (i + 4 + n > len) return -3;
        char *b = (char *) va_arg(arg, void *);
        osc_encode_uint32(n, (buffer + i)); i += 4;
        memcpy(buffer+i, b, n);
        i = (i + 3 + n) & ~0x3;
        break;
      }
      case 'f': {
        if (i + 4 > len) return -3;
        const float f = (float) va_arg(arg, double);
        osc_encode_uint32(f, (buffer + i));
        i += 4;
        break;
      }
      case 'i': {
        if (i + 4 > len) return -3;
        const uint32_t k = (uint32_t) va_arg(arg, int);
        osc_encode_uint32(k, (buffer + i));
        i += 4;
        break;
      }
      case 's': {
        const char *str = (const char *) va_arg(arg, void *);
        s_len = (int) strlen(str);
        if (i + s_len >= len) return -3;
        strncpy(buffer+i, str, len-i-s_len);
        i = (i + 4 + s_len) & ~0x3;
        break;
      }
      default: return -4;
    }
  }

  return i;
}

int osc_write_message(char *buffer, const int len, const char *address, const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  const int i = osc_vwrite_message(buffer, len, address, format, arg);
  va_end(arg);
  return i;
}