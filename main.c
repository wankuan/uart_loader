#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "link_mac.h"


uint8_t frame_payload[4] = {0x0, 0x01, 0x02, 0x03};


ret_t test_msg_cb(uint8_t *payload, uint16_t length)
{
    DBG("test callback has been called...\n");
    for(uint8_t index=0;index<length;++index){
        DBG("index:%d data:0x%x \n", index, payload[index]);
    }
    return RET_SUCCESS;
}

int main()
{
    frame_callback_table_init();
    register_frame_callback(0x11, 0x22, test_msg_cb);
    mac_frame_t *frame = malloc(sizeof(mac_frame_t) + sizeof(frame_payload));
    frame->preamble_sync  = 0x55555555;
    frame->payload_length = 8;
    frame->msg_class      = 0x11;
    frame->msg_id         = 0x22;
    memcpy(frame->payload, frame_payload, sizeof(frame_payload));
    frame->check_sum      = check_sum(frame->payload, frame->payload_length );

    unpack_stream_cycle((uint8_t *)frame, sizeof(mac_frame_t) + sizeof(frame_payload));

    return 0;
}
