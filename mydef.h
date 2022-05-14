#ifndef __MY_DEY_H__
#define __MY_DEY_H__

#include <stdio.h>

#define DBG(x,args...)  printf("[DEBUG] "x"", ##args)


typedef enum
{
    RET_SUCCESS = 0,
    RET_FAIL    = 1,
}ret_t;

#ifndef   __PACKED
  #define __PACKED                               __attribute__((packed, aligned(1)))
#endif


#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

#endif