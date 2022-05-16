#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t crc32_cal(uint8_t *data, uint32_t len, uint32_t init_crc32);

#ifdef __cplusplus
}
#endif

#endif