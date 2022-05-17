#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "link_mac.h"
#include "bin_transfer.hpp"
#include "bin_transfer_protocol.h"
#include "crc32.h"
#include "UartCore.hpp"


uint8_t frame_payload[4] = {0x0, 0x01, 0x02, 0x03};

ret_t test_msg_cb(mac_frame_t *frame)
{
    DBG("test callback has been called...msg_class:0x%x msg_id:0x%x length:%d\n", frame->msg_class, frame->msg_id, frame->payload_length);
    for (uint8_t index = 0; index < frame->payload_length; ++index) {
        DBG("index:%d data:0x%x \n", index, frame->payload[index]);
    }
    return RET_SUCCESS;
}

void write_data_to_app_bin(uint8_t *stream, uint32_t *stream_length) {
    static FILE *pFile = NULL;
    if (pFile == NULL) {
        pFile = fopen("app_bin_unpack.bin", "a+");
        DBG("open file...start to write..\n");
    }
    static uint32_t crc_result = 0xFFFFFFFF;
    *stream_length = fwrite(stream, 1, *stream_length, pFile);
    crc_result = crc32_cal(stream, *stream_length, crc_result);
    DBG("success write size:%d.. crc:0x%x\n", *stream_length, crc_result);
}

ret_t bin_loader_unpack_cb(mac_frame_t *frame)
{
    DBG("bin_loader_unpack_cb has been called...msg_class:0x%x msg_id:0x%x length:%d\n", frame->msg_class, frame->msg_id, frame->payload_length);
    data_transfer_req_t *req_t      = (data_transfer_req_t *)frame->payload;
    uint32_t             data_write = req_t->package_size;
    write_data_to_app_bin(req_t->package_buffer, &data_write);
    return RET_SUCCESS;
}

static void printf_all_data_ten_one_line(uint8_t *data, uint16_t size)
{
    uint8_t ten_bit = size / 10;
    uint8_t one_bit = size % 10;
    printf("size:%d ten:%d one:%d..\n", size, ten_bit, one_bit);
    for (uint8_t ten_index = 0; ten_index <= ten_bit; ++ten_index) {
        if (ten_index != ten_bit) {
            for (uint8_t one_index = 0; one_index < 10; one_index++) {
                printf("%.2X ", data[ten_index * 10 + one_index]);
            }
            printf("\n");
        } else {
            for (uint8_t one_index = 0; one_index < one_bit; one_index++) {
                printf("%X ", data[ten_index * 10 + one_index]);
            }
            printf("\n");
        }
    }
}

void read_data_from_file(const char *file_name, uint8_t *stream, uint32_t *stream_length, uint32_t buffer_size)
{
    FILE *pFile = NULL;
    pFile       = fopen(file_name, "r");
    if (pFile == NULL) {
        return;
    }
    *stream_length = fread(stream, 1, buffer_size, pFile);
    DBG("file:%s size:%d.. \n", file_name, *stream_length);
    printf_all_data_ten_one_line(stream, 200);
}
#define STREAM_MAX_LENGTH 522

void unpack_file_cycle(const char *file_name, uint16_t unpackage_size)
{
    FILE *pFile = NULL;
    pFile       = fopen(file_name, "r");
    if (pFile == NULL) {
        return;
    }
    uint8_t *stream_buffer     = (uint8_t *)malloc(unpackage_size);
    uint16_t success_read_size = 0;
    while (1) {
        memset(stream_buffer, 0, unpackage_size);
        success_read_size = fread(stream_buffer, 1, unpackage_size, pFile);
        unpack_stream_cycle((uint8_t *)stream_buffer, success_read_size);
        if (unpackage_size != success_read_size) {
            DBG("read file:%s done...\n", file_name);
            break;
        }
    }
}
int main(int argc, char *argv[])
{
    uint8_t mode = 0;
    if (argc <= 2) {
        // static uint32_t crc_result = 0xFFFFFFFF;
        // uint32_t stream = {0x00000001};
        // crc_result = crc32_cal(&stream, 4, crc_result);
        // DBG("scrc:0x%x\n", crc_result);

        DBG("no input args..\n");
        return -1;
    }
    if (!strcmp(argv[1], "test")) {
        DBG("======== self test =========\n");
        mode = 1;
    } else if (!strcmp(argv[1], "loader")) {
        DBG("======== MCU loader =========\n");
        mode = 2;
    } else {
        DBG("args[1] invalid..\n");
    }
    if (mode == 1) {
        if (argc != 4) {
            DBG("argc != 4..exit\n");
            return -1;
        }
        uint32_t one_unpack_size = atoi(argv[3]);
        if (one_unpack_size >= 1024) {
            DBG("one_unpack_size:%d over max:%d..exit\n", one_unpack_size, 1024);
            return -1;
        }
        frame_callback_table_init();
        register_frame_callback(0x11, 0x22, test_msg_cb);
        register_frame_callback(0x02, 0x04, bin_loader_unpack_cb);
        DBG("current binary file:%s...one_unpack_size:%d\n", argv[2], one_unpack_size);
        unpack_file_cycle(argv[2], one_unpack_size);
    } else if (mode == 2) {
#define MAX_UPGRADE_BIN_SIZE (1024 * 1024)
        DBG("upgrade binary file:%s...\n", argv[2]);
        uint8_t *input_max_buffer = (uint8_t *)malloc(MAX_UPGRADE_BIN_SIZE);
        memset(input_max_buffer, 0, MAX_UPGRADE_BIN_SIZE);
        uint32_t stream_length = 0;
        read_data_from_file(argv[2], input_max_buffer, &stream_length, MAX_UPGRADE_BIN_SIZE);

        auto uart0 = UartCore("/dev/cu.usbserial-23201", 921600);
        bin_transfer_entry(uart0, input_max_buffer, stream_length);
    }
    return 0;
}


