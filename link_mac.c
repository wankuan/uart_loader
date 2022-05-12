#include "link_mac.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define MAX_UNPACK_CB_SIZE (10)
#define MAX_FRAME_SIZE (522)
#define PREAMBLE_SYNC_DATA (0x55555555)


uint16_t check_sum(uint8_t *data, uint16_t length);

typedef struct {
    uint8_t msg_class;
    uint8_t msg_id;
    frame_unpack_callback_t callback;
} unpack_callback_item_t;

unpack_callback_item_t unpack_cb_item_table[10];

ret_t frame_callback_table_init(void) {
    for (uint8_t index = 0; index < ARRAY_SIZE(unpack_cb_item_table); index++) {
        unpack_cb_item_table[index].msg_class = 0;
        unpack_cb_item_table[index].msg_id = 0;
        unpack_cb_item_table[index].callback = NULL;
    }
    return RET_SUCCESS;
}

unpack_callback_item_t *find_frame_item(uint8_t msg_class, uint8_t msg_id) {
    for (uint8_t index = 0; index < ARRAY_SIZE(unpack_cb_item_table); index++) {
        if (unpack_cb_item_table[index].msg_class == msg_class && unpack_cb_item_table[index].msg_id == msg_id) {
            return &unpack_cb_item_table[index];
        }
    }
    return NULL;
}

ret_t register_frame_callback(uint8_t msg_class, uint8_t msg_id, frame_unpack_callback_t callback) {
    static               uint8_t index      = 0;
    unpack_cb_item_table[index].msg_class = msg_class;
    unpack_cb_item_table[index].msg_id    = msg_id;
    unpack_cb_item_table[index].callback  = callback;
    index ++;
    return RET_SUCCESS;
}


typedef enum {
    UNPACK_PREABLE_SYNC = 0,
    UNPACK_WAIT_DONE    = 1,
    UNPACK_FRAME_DONE   = 2,
    UNPACK_CHECK_SUM_OK = 3,
} unpack_fsm_step_t;

void printf_buffer(uint8_t *payload, uint16_t length)
{
    for(uint8_t index=0;index<length;++index){
        DBG("index:%d data:0x%x \n", index, payload[index]);
    }
}

ret_t unpack_stream_cycle(uint8_t *stream, uint16_t length) {
    static uint8_t unpack_inner_buffer[MAX_FRAME_SIZE];
    static uint16_t inner_buffer_index = 0;
    static unpack_fsm_step_t step = UNPACK_PREABLE_SYNC;
    static uint16_t last_frame_length_diff = 0;
    mac_frame_t *pFrame = NULL;
    uint8_t is_unpack_done = 0;
    uint8_t *pData = stream;
    uint16_t cur_read_stream_index = 0;

    pFrame = (mac_frame_t*)pData;
    // TODO: 判断当length<4的情况
    while (!is_unpack_done) {
        // 找到引导码，将后边的包全部送入buffer
        if (step == UNPACK_PREABLE_SYNC && *(uint32_t *)pData == PREAMBLE_SYNC_DATA) {
            DBG("new unpack buffer..\n");
            // 清空缓冲区，开始新的解包循环
            inner_buffer_index = 0;
            memset(unpack_inner_buffer, 0, sizeof(unpack_inner_buffer));
            // payload长度大于stream剩下的，把stream剩下的全部拷过来
            if((pFrame->payload_length + sizeof(mac_frame_t)) > (length - cur_read_stream_index)){
                memcpy((void*)unpack_inner_buffer, pData, length - cur_read_stream_index);
                // 单个frame尚未解包完成，将step置为check_sum
                inner_buffer_index = length - cur_read_stream_index;
                last_frame_length_diff = pFrame->payload_length + sizeof(mac_frame_t) - inner_buffer_index;
                step = UNPACK_WAIT_DONE;
                DBG("stream has been unpacked, wait for next stream... may be left:%d \n", last_frame_length_diff);
                // 退出解包循环
                break;
            }else{// payload长度小于stream剩下的，拷贝payload部分过来
                memcpy((void*)unpack_inner_buffer, pData, pFrame->payload_length + sizeof(mac_frame_t));
                inner_buffer_index     = pFrame->payload_length + sizeof(mac_frame_t);
                pData                 += pFrame->payload_length + sizeof(mac_frame_t);
                cur_read_stream_index += pFrame->payload_length + sizeof(mac_frame_t);
                // 单个包解析完毕
                step = UNPACK_FRAME_DONE;
                DBG("one frame unpacked done...\n");
            }

        }
        // 这个stream会包含上个没有解完的包
        if(step == UNPACK_WAIT_DONE){
            DBG("continue unpack last wait done frame...\n");
            if(length >= last_frame_length_diff){ // frame内解完整个包
                memcpy((void*)&unpack_inner_buffer[inner_buffer_index], pData, length - last_frame_length_diff);
                inner_buffer_index    += length - last_frame_length_diff;
                pData                 += length - last_frame_length_diff;
                cur_read_stream_index += length - last_frame_length_diff;
                // 单个包解析完毕
                step = UNPACK_FRAME_DONE;
                DBG("one frame unpacked done...\n");
            }else{ // 还是没解完
                memcpy((void*)&unpack_inner_buffer[inner_buffer_index], pData, length - cur_read_stream_index);
                // 单个frame尚未解包完成，将step置为check_sum
                inner_buffer_index     = length - cur_read_stream_index;
                last_frame_length_diff = pFrame->payload_length + sizeof(mac_frame_t) - inner_buffer_index;
                step                   = UNPACK_WAIT_DONE;
                DBG("stream has been unpacked, wait for next stream... may be left:%d \n", last_frame_length_diff);
                // 退出解包循环
                break;
            }
        }
        // 解包完成,开始校验数据
        if(step == UNPACK_FRAME_DONE){
            pFrame = (mac_frame_t*)unpack_inner_buffer;
            DBG("===output inner buffer...\n");
            printf_buffer(unpack_inner_buffer, inner_buffer_index);
            DBG("===output payload buffer...\n");
            printf_buffer(pFrame->payload, pFrame->payload_length);
            if(pFrame->check_sum == check_sum(pFrame->payload, pFrame->payload_length)){
                DBG("====== FRAME CHECK SUM OK =======\n");
                step = UNPACK_CHECK_SUM_OK;
            }else{ // TODO:校验失败，暂时不考虑
                DBG("!!!!  FRAME CHECK SUM FAIL !!!!\n");
                step = UNPACK_PREABLE_SYNC;
                continue;;
            }
        }
        // 校验通过，调用回调函数
        if(step == UNPACK_CHECK_SUM_OK){
            unpack_callback_item_t *item = find_frame_item(pFrame->msg_class, pFrame->msg_id);
            if(item == NULL){
                DBG("item nullptr...\n");
            }else{
                if(item->callback != NULL){
                    item->callback(pFrame->payload, pFrame->payload_length);
                }else{
                    DBG("can not find match item...callback not register...\n");
                }
            }

            // 开启新一轮解包循环
            step = UNPACK_PREABLE_SYNC;
            continue;
        }

        if (cur_read_stream_index++ >= length) {
            is_unpack_done = 1;
        }
        pData++;
    }
    return RET_SUCCESS;
}

uint16_t check_sum(uint8_t *data, uint16_t length)
{
    uint8_t CK_A = 0;
    uint8_t CK_B = 0;
    uint16_t result = 0;
    for(uint16_t index = 0; index<length; ++index){
        CK_A = CK_A + data[index];
        CK_B = CK_B + CK_A;
    }
    result = (CK_B << 8) + CK_A;
    DBG("check_sum:0x%x..\n", result);
    return result;
}


