#include "link_mac.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_UNPACK_CB_SIZE (10)
#define PREAMBLE_SYNC_DATA (0x55555555)


uint16_t check_sum(uint8_t *data, uint16_t length);

typedef struct
{
    uint8_t                 msg_class;
    uint8_t                 msg_id;
    frame_unpack_callback_t callback;
} unpack_callback_item_t;

unpack_callback_item_t unpack_cb_item_table[10];

ret_t frame_callback_table_init(void)
{
    for (uint8_t index = 0; index < ARRAY_SIZE(unpack_cb_item_table); index++) {
        unpack_cb_item_table[index].msg_class = 0;
        unpack_cb_item_table[index].msg_id    = 0;
        unpack_cb_item_table[index].callback  = NULL;
    }
    return RET_SUCCESS;
}

unpack_callback_item_t *find_frame_item(uint8_t msg_class, uint8_t msg_id)
{
    for (uint8_t index = 0; index < ARRAY_SIZE(unpack_cb_item_table); index++) {
        if (unpack_cb_item_table[index].msg_class == msg_class && unpack_cb_item_table[index].msg_id == msg_id) {
            return &unpack_cb_item_table[index];
        }
    }
    return NULL;
}

ret_t register_frame_callback(uint16_t msg_class_id, frame_unpack_callback_t callback)
{
    static uint8_t index                  = 0;
    unpack_cb_item_table[index].msg_class = MSG_CLASS_MASK(msg_class_id);
    unpack_cb_item_table[index].msg_id    = MSG_ID_MASK(msg_class_id);
    unpack_cb_item_table[index].callback  = callback;
    index++;
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
    for (uint8_t index = 0; index < length; ++index) {
        DBG("index:%d data:0x%x \n", index, payload[index]);
    }
}

ret_t pack_one_frame(uint16_t msg_class_id, uint8_t type, uint8_t *payload, uint16_t payload_length, mac_frame_t *frame_buffer,
                     uint16_t *buffer_size)
{
    mac_frame_t *frame = (mac_frame_t *)frame_buffer;
    if ((sizeof(mac_frame_t) + payload_length) > *buffer_size) {
        ERROR("frame buffer size:%d has not cover sizeof(mac_frame_t) + payload_length\n", *buffer_size);
        *buffer_size = 0;
        return RET_FAIL;
    }
    *buffer_size          = sizeof(mac_frame_t) + payload_length;
    frame->preamble_sync  = PREAMBLE_SYNC_DATA;
    frame->payload_length = payload_length;
    frame->msg_class      = MSG_CLASS_MASK(msg_class_id);
    frame->msg_id         = MSG_ID_MASK(msg_class_id);
    frame->type           = type;
    frame->check_sum      = check_sum(payload, payload_length);
    memcpy(frame->payload, payload, payload_length);
    DBG("pack_one_frame success.. size:%d msg_class:%d msg_id:%d type:%d payload_length:%d\n", *buffer_size, frame->msg_class, frame->msg_id,
        frame->type, frame->payload_length);
    return RET_SUCCESS;
}

/**
 * 解包思路：
 * step维护全局的当前解包状态，根据step进入不同的解包逻辑
 * 对于最后stream的长度不满足sizeof(mac_frame_t)的，拷贝到缓冲区，并置位flag
 */

/**
 * @brief
 *
 * @param stream
 * @param length
 * @return ret_t
 */
ret_t unpack_stream_cycle(uint8_t *stream, uint16_t stream_length)
{
    // frame_buffer 是一个完整的mac_frame_t buffer，如果index大于sizeof(mac_frame_t)则可用
    static uint8_t           frame_buffer[MAX_LINK_BUFFER_PAYLOAD_LENGTH] = {0};
    static uint16_t          frame_buffer_index                           = 0;
    static uint16_t          last_frame_left_length                       = 0;
    static unpack_fsm_step_t step                                         = UNPACK_PREABLE_SYNC;

    // debug 信息统计
    static uint16_t total_stream_nums      = 0;
    static uint16_t total_stream_fail_nums = 0;

    mac_frame_t *pFrame                = (mac_frame_t *)frame_buffer;  // 上一个包前导码+payloadlength不完整
    uint8_t     *pData                 = stream;
    uint16_t     cur_stream_read_index = 0;
    if (total_stream_nums == 57) {
        total_stream_nums = 57;
    }
    DBG("\n\n=========stream unpack cycle..stream_length:%d frame_buffer_index:%d total:%d\n", stream_length, frame_buffer_index, total_stream_nums);
    total_stream_nums++;

    // 前置判断
    if (stream == NULL || stream_length == 0) {
        ERROR("%s stream:%p, stream_length:%d ..\n", __FILE__, stream, stream_length);
    }

    uint8_t is_unpack_done = 0;
    while (!is_unpack_done) {
        // 大部分遇到的情况，上个stream没有完全解出整个frame，现在继续解
        if (step == UNPACK_PREABLE_SYNC) {
            // 处理frame header不满足的情况
            if (frame_buffer_index < sizeof(mac_frame_t)) {
                // 剩下的stream满足整个frame header，拷贝header出来
                if ((stream_length - cur_stream_read_index) >= sizeof(mac_frame_t)) {
                    uint16_t header_diff = 0;
                    // TODO: 需要考虑frame_buffer_index越界的情况
                    DBG("cover header...frame_buffer_index:%d cur_stream_read_index:%d  header_diff:%d...\n", frame_buffer_index,
                        cur_stream_read_index, header_diff);
                    header_diff = sizeof(mac_frame_t) - frame_buffer_index;
                    memcpy((void *)&frame_buffer[frame_buffer_index], (void *)&stream[cur_stream_read_index], header_diff);
                    frame_buffer_index += header_diff;
                    cur_stream_read_index += header_diff;
                    last_frame_left_length = pFrame->payload_length;
                } else {  // / 剩下的stream 还不是个完整的header，拷贝到frame buffer后退出
                    DBG("current frame_buffer_index:%d is invalid..\n", frame_buffer_index);
                    // TODO: 需要考虑frame_buffer_index越界的情况
                    memcpy((void *)&frame_buffer[frame_buffer_index], (void *)&stream[cur_stream_read_index], stream_length - cur_stream_read_index);
                    frame_buffer_index += stream_length - cur_stream_read_index;
                    if (frame_buffer_index >= MAX_LINK_BUFFER_PAYLOAD_LENGTH) {
                        // TODO: 异常，状态机复位实现
                        ERROR("frame_buffer_index:%d over %ld..\n", frame_buffer_index, MAX_LINK_BUFFER_PAYLOAD_LENGTH);
                    }
                    DBG("stream left stream_length:%d not cover while frame header:%lu..exit\n", (stream_length - cur_stream_read_index),
                        sizeof(mac_frame_t));
                    break;
                }
            }
            if (pFrame->preamble_sync != PREAMBLE_SYNC_DATA) {
                ERROR("frame preamble_sync:0x%x not match:0x%x ..\n", pFrame->preamble_sync, PREAMBLE_SYNC_DATA);
            }
            DBG("new frame buffer unpack..payload_length:%d\n", pFrame->payload_length);
            DBG("last_frame_left_length:%d stream_length:%d cur_stream_read_index:%d frame_buffer_index:%d\n", last_frame_left_length, stream_length,
                cur_stream_read_index, frame_buffer_index);
            // frame所需的长度还是大于stream剩下的，把stream剩下的全部拷过来
            if ((last_frame_left_length) > (stream_length - cur_stream_read_index)) {
                // 更新frame所需的长度

                // TODO: 需要考虑frame_buffer_index越界的情况
                memcpy((void *)&frame_buffer[frame_buffer_index], (void *)&stream[cur_stream_read_index], stream_length - cur_stream_read_index);
                last_frame_left_length -= stream_length - cur_stream_read_index;
                frame_buffer_index += stream_length - cur_stream_read_index;

                DBG("unpack done, not a whole frame, wait for next stream...cur_frame_buffer_index:%d left:%d \n", frame_buffer_index,
                     last_frame_left_length);
                // 退出解包循环
                break;
            } else {  // frame所需的长度还是满足stream剩下的，拷贝payload部分过来
                // TODO: 需要考虑frame_buffer_index越界的情况
                memcpy((void *)&frame_buffer[frame_buffer_index], (void *)&stream[cur_stream_read_index], last_frame_left_length);
                frame_buffer_index += last_frame_left_length;
                cur_stream_read_index += last_frame_left_length;
                // 单个包解析完毕
                step = UNPACK_FRAME_DONE;
                DBG("one frame unpacked done...\n");
            }
        }
        // 解包完成,开始校验数据
        if (step == UNPACK_FRAME_DONE) {
            // DBG("===output payload buffer...\n");
            // printf_buffer(pFrame->payload, pFrame->payload_length);
            if (pFrame->check_sum == check_sum(pFrame->payload, pFrame->payload_length)) {
                DBG("====== FRAME CHECK SUM OK =======class:0x%x id:0x%x\n", pFrame->msg_class, pFrame->msg_id);
                step = UNPACK_CHECK_SUM_OK;
            } else {  // TODO:校验失败，暂时不考虑
                ERROR("!!!!  FRAME CHECK SUM FAIL !!!!\n");
                step = UNPACK_PREABLE_SYNC;
                continue;
            }
        }
        // 校验通过，调用回调函数
        if (step == UNPACK_CHECK_SUM_OK) {
            unpack_callback_item_t *item = find_frame_item(pFrame->msg_class, pFrame->msg_id);
            if (item == NULL) {
                ERROR("item nullptr...class:0x%x id:0x%x..\n", pFrame->msg_class, pFrame->msg_id);
            } else {
                if (item->callback != NULL) {
                    item->callback(pFrame);
                } else {
                    ERROR("can not find match item...callback not register...class:0x%x id:0x%x..\n", pFrame->msg_class, pFrame->msg_id);
                }
            }

            step = UNPACK_PREABLE_SYNC;
            // 清空缓冲区，开始新的解包循环
            frame_buffer_index = 0;
            memset(frame_buffer, 0, sizeof(frame_buffer));
            continue;
        }

        // 正常情况下，不会执行到这里，若是则出错了
        ERROR("link unpack FSM error...\n");
    }
    return RET_SUCCESS;
}

uint16_t check_sum(uint8_t *data, uint16_t length)
{
    uint8_t  CK_A   = 0;
    uint8_t  CK_B   = 0;
    uint16_t result = 0;
    for (uint16_t index = 0; index < length; ++index) {
        CK_A = CK_A + data[index];
        CK_B = CK_B + CK_A;
    }
    result = (CK_B << 8) + CK_A;
    DBG("check_sum:0x%x..\n", result);
    return result;
}
