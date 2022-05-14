#ifndef __LINK_MAC_H__
#define __LINK_MAC_H__

#include <stdint.h>
#include "mydef.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MSG_CLASS_MASK(x) (x>>8)
#define MSG_ID_MASK(x) ((uint16_t) x & 0x00FF)

typedef struct
{
    uint32_t    preamble_sync;  // 0x55 0x55 0x55 0x55
    uint16_t    payload_length; // max 65536
    uint16_t    check_sum;      // include msg_class + msg_id + payload_length + payload
    uint8_t     msg_class;
    uint8_t     msg_id;
    uint8_t     payload[0]; // offset 10
}__PACKED mac_frame_t;

// typedef struct
// {
//     uint8_t     msg_class;
//     uint8_t     msg_id;
//     uint16_t    payload_length; // max 65536
//     uint8_t     *payload_buffer;
// }__PACKED app_frame_t;


// typedef struct
// {

// };


typedef ret_t (*frame_unpack_callback_t)(mac_frame_t *frame);

ret_t frame_callback_table_init(void);
ret_t register_frame_callback(uint8_t msg_class, uint8_t msg_id, frame_unpack_callback_t callback);

ret_t unpack_stream_cycle(uint8_t *stream, uint16_t length);

// 打包一个frame
/**
 * @brief
 *
 * @param msg_class_id class+id的组合
 * @param payload payload数组指针
 * @param payload_length payload长度
 * @param frame_buffer frame buffer的指针
 * @param buffer_size frame buffer的长度，必须大于sizeof(frame) + payload_length，in:开的buffer长度， out：打包后的frame长度
 * @return ret_t 0:success 1:fail
 */
ret_t pack_one_frame(uint16_t msg_class_id, uint8_t *payload, uint16_t payload_length,
                    mac_frame_t *frame_buffer, uint16_t *buffer_size);

// unpack frame
// unpack function -> need to called period
// ret_t unpack_stream_cycle(uint8_t *stream, uint16_t length);


// // callback handle
// ret_t frame_callback_table_init(void);
// ret_t register_frame_callback(uint8_t msg_class, uint8_t msg_id, frame_unpack_callback_t callback);
// frame_unpack_callback_t find_frame_callback(uint8_t msg_class, uint8_t msg_id);

// // pack frame & send
// ret_t mac_frame_write_data(mac_frame_t *frame, uint8_t msg_class, uint8_t msg_id, uint8_t *data);
// ret_t mac_fill_check_sum(mac_frame_t *frame);
// ret_t send_mac_frame(void);
uint16_t check_sum(uint8_t *data, uint16_t length);


#ifdef __cplusplus
}
#endif

#endif


