#include "uart_port.hpp"
#include <stdint.h>
#include <stdio.h>
// #include <stdint.h>


void printf_all_data_ten_one_line(uint8_t *data, uint16_t size)
{
    uint8_t ten_bit = size/10;
    uint8_t one_bit = size%10;
    printf("size:%d ten:%d one:%d..\n", size, ten_bit, one_bit);
    for(uint8_t ten_index = 0; ten_index <= ten_bit; ++ten_index){

        if(ten_index != ten_bit){
            for(uint8_t one_index = 0; one_index<10; one_index++){
                printf("%.2X ", data[ten_index*10 + one_index]);
            }
            printf("\n");
        }else
        {
            for(uint8_t one_index = 0; one_index<one_bit; one_index++){
                printf("%X ", data[ten_index*10 + one_index]);
            }
            printf("\n");
        }

    }
}
void write_data_to_file(uint8_t *stream, uint16_t *stream_length) {
    static FILE *pFile = NULL;
    if (pFile == NULL) {
        pFile = fopen("test_bin_package", "a+");
        DBG("open file...start to write..\n");
    }
    *stream_length = fwrite(stream, 1, *stream_length, pFile);
    DBG("success write size:%d.. \n", *stream_length);
}

/**
 * @brief  同步：传输数据到指定端口
 *
 * @param port_num
 * @param data
 * @param data_size
 * @param timeout
 * @return ret_t
 */
ret_t transfer_data_to_port_sync(uint8_t port_num, uint8_t *data, uint16_t *data_size, uint32_t timeout)
{
    // TODO: 实现串口发送，将数据输出到bin中，用解包函数再解析一次
    DBG("transfer_data_to_port_sync ... \n");
    write_data_to_file(data, data_size);
    printf_all_data_ten_one_line(data, *data_size);
    return RET_SUCCESS;
}

