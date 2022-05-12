#ifndef __MY_DEY_H__
#define __MY_DEY_H__

#include <stdio.h>

#define DBG(x,args...)  printf("[DEBUG] "x"", ##args)


#ifndef   __PACKED
  #define __PACKED                               __attribute__((packed, aligned(1)))
#endif


#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

#endif