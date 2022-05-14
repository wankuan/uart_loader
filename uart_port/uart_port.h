#ifndef __UART_PORT_H__
#define __UART_PORT_H__

#include "mydef.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

ret_t transfer_data_to_port_sync(uint8_t port_num, uint8_t *data, uint16_t *data_size, uint32_t timeout);


#ifdef __cplusplus
}
#endif

#endif