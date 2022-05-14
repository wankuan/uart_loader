#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "link_mac.h"
#include "bin_transfer.h"

uint8_t frame_payload[4] = {0x0, 0x01, 0x02, 0x03};

ret_t test_msg_cb(mac_frame_t *frame) {
    DBG("test callback has been called...msg_class:0x%x msg_id:0x%x length:%d\n", frame->msg_class, frame->msg_id,
        frame->payload_length);
    for (uint8_t index = 0; index < frame->payload_length; ++index) {
        DBG("index:%d data:0x%x \n", index, frame->payload[index]);
    }
    return RET_SUCCESS;
}


static void printf_all_data_ten_one_line(uint8_t *data, uint16_t size)
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

void read_data_from_file(const char *file_name, uint8_t *stream, uint32_t *stream_length, uint32_t buffer_size) {
    FILE *pFile = NULL;
    pFile = fopen(file_name, "r");
    if (pFile == NULL) {
        return;
    }
    *stream_length = fread(stream, 1, buffer_size, pFile);
    DBG("file:%s size:%d.. \n", file_name, *stream_length);
    printf_all_data_ten_one_line(stream, 200);
}
#define STREAM_MAX_LENGTH 522


int main(int argc, char *argv[]) {
    uint8_t mode = 0;
    if (argc <= 2) {
        DBG("no input args..\n");
        return -1;
    }
    if(!strcmp(argv[1], "test")){
        DBG("======== self test =========\n");
        mode = 1;
    }else if(!strcmp(argv[1], "loader")){
        DBG("======== MCU loader =========\n");
        mode = 2;
    }else{
        DBG("args[1] invalid..\n");
    }
    if(mode == 1){
        uint8_t input_max_buffer[STREAM_MAX_LENGTH] = {0};
        uint32_t stream_length = 0;
        frame_callback_table_init();
        register_frame_callback(0x11, 0x22, test_msg_cb);
        for (uint8_t i = 2; i < argc; ++i) {
            DBG("current binary file:%s...\n", argv[i]);
            memset(input_max_buffer, 0, STREAM_MAX_LENGTH);
            stream_length = 0;
            read_data_from_file(argv[i], input_max_buffer, &stream_length, STREAM_MAX_LENGTH);

            unpack_stream_cycle((uint8_t *)input_max_buffer, stream_length);
        }
    }else if(mode == 2){
        #define MAX_UPGRADE_BIN_SIZE (1024*1024)
        DBG("upgrade binary file:%s...\n", argv[2]);
        uint8_t *input_max_buffer = (uint8_t *)malloc(MAX_UPGRADE_BIN_SIZE);
        memset(input_max_buffer, 0, MAX_UPGRADE_BIN_SIZE);
        uint32_t stream_length = 0;
        read_data_from_file(argv[2], input_max_buffer, &stream_length, MAX_UPGRADE_BIN_SIZE);

        bin_transfer_entry(input_max_buffer, stream_length);
    }

    // mac_frame_t *frame = malloc(sizeof(mac_frame_t) + sizeof(frame_payload));
    // frame->preamble_sync  = 0x55555555;
    // frame->payload_length = 4;
    // frame->msg_class      = 0x11;
    // frame->msg_id         = 0x22;
    // memcpy(frame->payload, frame_payload, sizeof(frame_payload));
    // frame->check_sum      = check_sum(frame->payload, frame->payload_length );

    // unpack_stream_cycle((uint8_t *)frame, sizeof(mac_frame_t) + sizeof(frame_payload));

    return 0;
}
