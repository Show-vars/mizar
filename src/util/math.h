#ifndef _H_UTIL_MATH_
#define _H_UTIL_MATH_

#ifndef max
  #define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
  #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef clamp
  #define clamp(val,minval,maxval) ((val > maxval) ? maxval : ((val < minval) ? minval : val))
#endif

#endif