#pragma once

#include <stdint.h>
#include <vector>
#include <mutex>
#include <termios.h>

#define DEFAULT_UART_BAUDRATE (921600)

class UartCore
{
public:
    // UartCore(const char* port_name);
    UartCore(const char* port_name, uint32_t baudrate);
    uint32_t SendData(uint8_t *data, uint32_t size);
    uint32_t ReadData(uint8_t *data, uint32_t size);
    ~UartCore();

private:
    const char* m_port_name;
    int         m_fd;
    uint32_t    m_baudrate = DEFAULT_UART_BAUDRATE;
    std::mutex  m_write_mutex;
    std::mutex  m_read_mutex;
    termios     m_port_config;
    // std::mutex m_write_mutex;
};
