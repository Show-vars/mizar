#include <stdint.h>

#ifndef _H_PCM_
#define _H_PCM_

/*
  Audio format pattern (32-bit unsigned):
    Bit    Size   Desc
    0      1      endian   (0 little, 1 big)
    1      1      signed   (0-1)
    2      1      reserved
    3-5    3      depth    (0-7: 8, 16, 24, 32, ...)
    6-23   18     rate     (0-262143)
    24-31  8      channels (0-255)
*/

typedef uint32_t audio_format_t;

#define AF_ENDIAN_MASK    0x00000001
#define AF_SIGNED_MASK    0x00000002
#define AF_DEPTH_MASK     0x00000038
#define AF_RATE_MASK      0x00ffffc0
#define AF_CHANNELS_MASK  0xff000000

#define AF_ENDIAN_SHIFT   0
#define AF_SIGNED_SHIFT   1
#define AF_DEPTH_SHIFT    0
#define AF_RATE_SHIFT     6
#define AF_CHANNELS_SHIFT 24

#define af_get_endian(af)       ( ((af) & AF_ENDIAN_MASK      ) >> AF_ENDIAN_SHIFT )
#define af_get_signed(af)       ( ((af) & AF_SIGNED_MASK      ) >> AF_SIGNED_SHIFT )
#define af_get_depth(af)        ( ((af) & AF_DEPTH_MASK       ) >> AF_DEPTH_SHIFT )
#define af_get_rate(af)         ( ((af) & AF_RATE_MASK        ) >> AF_RATE_SHIFT )
#define af_get_channels(af)     ( ((af) & AF_CHANNELS_MASK    ) >> AF_CHANNELS_SHIFT )

#define af_endian(val)          ( ((val) << AF_ENDIAN_SHIFT   ) & AF_ENDIAN_MASK )
#define af_signed(val)          ( ((val) << AF_SIGNED_SHIFT   ) & AF_SIGNED_MASK )
#define af_depth(val)           ( ((val) << AF_DEPTH_SHIFT    ) & AF_DEPTH_MASK )
#define af_rate(val)            ( ((val) << AF_RATE_SHIFT     ) & AF_RATE_MASK )
#define af_channels(val)        ( ((val) << AF_CHANNELS_SHIFT ) & AF_CHANNELS_MASK )

#define af_get_sample_size(af)    ( af_get_depth((af))       >> 3 )
#define af_get_frame_size(af)	    ( af_get_sample_size((af)) * af_get_channels((af)) )
#define af_get_second_size(af)    ( af_get_rate((af))        * af_get_frame_size((af)) )

#endif