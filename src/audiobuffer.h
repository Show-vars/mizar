#ifndef _H_AUDIOBUFFER_
#define _H_AUDIOBUFFER_

#include <stdint.h>
#include <uv.h>
#include "pcm.h"

/**
 * Audio ring buffer implementation.
 * 
 * This buffer can only hold array of float values.
 */

typedef struct {
  audio_format_t af;
  size_t capacity;
  
  // last consume frame number
  volatile uint64_t frames;

  // all indexes and ofsets in pcm samples
  volatile size_t available;
  volatile size_t read_index;
  volatile size_t write_index;

  uv_mutex_t mutex;

  // read/write begin/end state
  uint8_t r_begin;
  uint8_t w_begin;
  size_t r_available, r_max_samples, r_index, r_count;
  size_t w_available, w_max_samples, w_index, w_count;

  float data[];
} audiobuffer_t;

/**
 * Audio ring buffer initialization
 *
 * @param af Audio format that our buffer will hold
 * @param frames Buffer capacity in PCM frames
 * @return Pointer to initialized buffer
 */
audiobuffer_t* audiobuffer_create(audio_format_t af, size_t frames);

/**
 * Free memory
 *
 * @param b Pointer to initialized buffer
 */
void audiobuffer_destroy(audiobuffer_t* b);

/**
 * Reset buffer. This function reset all counter and buffer become empty
 *
 * @param b Pointer to initialized buffer
 */
void audiobuffer_reset(audiobuffer_t* b);

/**
 * Get last consumed frame number
 * 
 * This function blocks until data became accessible
 *
 * @param b Pointer to initialized buffer
 * @return Last consumed frame number
 */
uint64_t audiobuffer_get_frames(audiobuffer_t* b);

/**
 * Set number of last consumed frame. Used exclusively for seeking over audio source
 * 
 * This function blocks until data became accessible
 *
 * @param b Pointer to initialized buffer
 */
void audiobuffer_set_frames(audiobuffer_t* b, uint64_t frame);

/**
 * Begin reading routine
 * 
 * This function blocks until data became accessible
 *
 * @param b Pointer to initialized buffer
 * @param max_frames Maximum frames number that can be processed in this routine
 * @return Number of frames available for reading
 */
size_t audiobuffer_read_begin(audiobuffer_t* b, const size_t max_frames);

/**
 * Prepare pointer with samples for reading
 *
 * @param b Pointer to initialized buffer
 * @param ptr Returned pointer to data
 * @return Number of frames that can be read from provided pointer
 */
size_t audiobuffer_read(audiobuffer_t* b, float** ptr);

/**
 * Mark specified frames as consumed
 *
 * @param b Pointer to initialized buffer
 * @param frames Frames that will be marked as consumed
 * @return Number of marked frames from beginning of current read routine
 */
size_t audiobuffer_read_consume(audiobuffer_t* b, const size_t frames);

/**
 * End current read routine
 * 
 * This function blocks until data became accessible
 *
 * @param b Pointer to initialized buffer
 * @return Number of frames was proccessed by current read routine
 */
size_t audiobuffer_read_end(audiobuffer_t* b);

/**
 * Begin writing routine
 * 
 * This function blocks until data became accessible
 *
 * @param b Pointer to initialized buffer
 * @param max_frames Maximum number of frames number that can be processed in this routine
 * @return Free space available for writing in frames 
 */
size_t audiobuffer_write_begin(audiobuffer_t* b, const size_t max_frames);

/**
 * Prepare pointer for writing samples
 *
 * @param b Pointer to initialized buffer
 * @param ptr Returned pointer to data
 * @return Number of frames that can be written to the provided pointer
 */
size_t audiobuffer_write(audiobuffer_t* b, float** ptr);

/**
 * Mark specified frames as filled
 *
 * @param b Pointer to initialized buffer
 * @param frames Frames that will be marked as filled
 * @return Number of marked frames from beginning of current write routine
 */
size_t audiobuffer_write_fill(audiobuffer_t* b, const size_t frames);

/**
 * End current write routine
 * 
 * This function blocks until data became accessible
 *
 * @param b Pointer to initialized buffer
 * @return Number of frames was proccessed by current write routine
 */
size_t audiobuffer_write_end(audiobuffer_t* b);

#endif