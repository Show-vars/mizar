#ifndef _H_UTIL_MATH_
#define _H_UTIL_MATH_

#ifndef max
  #define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define max4(a,b,c,d) (max(a,max(b,max(c,d))))

#ifndef min
  #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define min4(a,b,c,d) (min(a,min(b,min(c,d))))

#ifndef clamp
  #define clamp(val,minval,maxval) ((val > maxval) ? maxval : ((val < minval) ? minval : val))
#endif

#endif