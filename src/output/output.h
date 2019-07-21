#ifndef _H_OUTPUT_
#define _H_OUTPUT_

#include <stdint.h>
#include "pcm.h"

struct output_device {
	int (*init)(void);
	int (*destroy)(void);
	int (*open)(audio_format_t af);
	int (*close)(void);
	int (*drop)(void);
	size_t (*write)(const uint8_t *buf, size_t frames);
	size_t (*buffer_space)(void);
	int (*pause)(void);
	int (*unpause)(void);
};

const struct output_device output_device_ops;

#endif