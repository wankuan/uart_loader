#include <UartCore.hpp>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "mydef.h"


UartCore::UartCore(const char* port_name, uint32_t baudrate):m_port_name(port_name),m_baudrate(baudrate)
{
    m_fd = open(m_port_name, O_RDWR | O_NOCTTY);

    if (m_fd < 0)
        DBG("open %s device fail..\n", port_name);

    tcgetattr(m_fd, &m_port_config); /* Get the current attributes of the Serial port */

    /* Setting the Baud rate */
    cfsetispeed(&m_port_config, baudrate); /* Set Read  Speed as 9600                       */
    cfsetospeed(&m_port_config, baudrate); /* Set Write Speed as 9600                       */

    /* 8N1 Mode */
    m_port_config.c_cflag &= ~PARENB; /* Disables the Parity Enable bit(PARENB),So No Parity   */
    m_port_config.c_cflag &= ~CSTOPB; /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
    m_port_config.c_cflag &= ~CSIZE;  /* Clears the mask for setting the data size             */
    m_port_config.c_cflag |= CS8;     /* Set the data bits = 8                                 */

    m_port_config.c_cflag &= ~CRTSCTS;       /* No Hardware flow Control                         */
    m_port_config.c_cflag |= CREAD | CLOCAL; /* Enable receiver,Ignore Modem Control lines       */

    m_port_config.c_iflag &= ~(IXON | IXOFF | IXANY);         /* Disable XON/XOFF flow control both i/p and o/p */
    m_port_config.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* Non Cannonical mode                            */

    m_port_config.c_oflag &= ~OPOST; /*No Output Processing*/

    /* Setting Time outs */
    m_port_config.c_cc[VMIN]  = 10; /* Read at least 10 characters */
    m_port_config.c_cc[VTIME] = 1;  /* Wait indefinetly  unit:100ms */

    if ((tcsetattr(m_fd, TCSANOW, &m_port_config)) != 0) /* Set the attributes to the termios structure*/
        DBG("set port attribtue fail...\n ");
    else
        DBG("Success set %s baudrate:%d ...\n", m_port_name, m_baudrate);
    tcflush(m_fd, TCIFLUSH); /* Discards old data in the rx buffer            */

}
UartCore::~UartCore()
{

}

uint32_t UartCore::ReadData(uint8_t *data, uint32_t size)
{
    static uint32_t total_size = 0;
    std::lock_guard<std::mutex> lk(m_read_mutex);
    size = read(m_fd, data, size);
    total_size += size;
    DBG("ReadData size:%d..total:%u\n", size, total_size);
    return size;
}



uint32_t UartCore::SendData(uint8_t *data, uint32_t size)
{
    static uint32_t total_size = 0;
    std::lock_guard<std::mutex> lk(m_write_mutex);
    size = write(m_fd, data, size);
    total_size += size;
    DBG("SendData size:%d..total:%u\n", size, total_size);
    return size;
}