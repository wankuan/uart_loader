#ifndef __BIN_TRANSFER_H__
#define __BIN_TRANSFER_H__

#include "mydef.h"
#include "UartCore.hpp"

void bin_transfer_entry(UartCore &uart_device, uint8_t *bin_buffer, uint32_t bin_size);

#endif