#ifndef __BIN_TRANSFER_PROTOCOL_H__
#define __BIN_TRANSFER_PROTOCOL_H__

#include <stdint.h>
#include "mydef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ret_t ret_code_t;

typedef struct
{
    uint8_t AA;
    uint8_t BB;
    uint8_t CC;
    uint8_t DD;
    char version_str[16]; // 最后一个字符必须'是\0'
}version_info_t;

// class:0x01 id:0x00 查询固件的版本号信息
#define GET_VERSION_REQUEST (0x0100)
// payload信息
typedef struct
{
    uint8_t reserved;
}__PACKED get_version_req_t;
typedef struct
{
    ret_code_t ret_code;
    version_info_t version;
}__PACKED get_version_ack_t;

// 固件传输payload定义

// class:0x02 id:0x00 请求开始进行固件升级
#define REQUEST_INTO_UPGRADE (0x0200)
// payload信息
typedef struct
{
    uint8_t reserved;
}__PACKED into_upgrade_req_t;
typedef struct
{
    ret_code_t ret_code;
}__PACKED into_upgrade_ack_t;

// class:0x02 id:0x02 请求进入loader
#define REQUEST_INTO_LOADER (0x0202)
// payload信息
typedef struct
{
    uint8_t reserved;
}__PACKED into_loader_req_t;
typedef struct
{
    ret_code_t ret_code;
}__PACKED into_loader_ack_t;

// class:0x02 id:0x03 进入loader成功，传输升级配置信息
#define REQUEST_TRANSFER_CONFIG (0x0203)
// payload信息
typedef struct
{
    uint32_t max_bin_size;
    uint32_t one_package_size; //  正常情况下为512
    uint32_t max_one_package_write_timeout; // uint:ms
}__PACKED transfer_config_req_t;
typedef struct
{
    ret_code_t ret_code;
    uint32_t   bin_total_size;  // 固件大小，uint:byte
    uint32_t   one_package_size; // 单个固件切片，uint:byte
    uint32_t   package_nums;  // 固件切片总数
}__PACKED transfer_config_ack_t;


// class:0x02 id:0x04 传输固件固件切片
#define DATA_TRANSFER_REQUEST (0x0204)
// payload信息
typedef struct
{
    uint32_t cur_package_index;
    uint32_t  package_size;
    uint8_t  package_buffer[0];
}__PACKED data_transfer_req_t;
typedef struct
{
    ret_code_t ret_code; // 非0，发起重传
    uint32_t   nex_package_index; // 指定下一个要传输的包index，正常情况下为cur_package_index+1
}__PACKED data_transfer_ack_t;


// class:0x02 id:0x05 升级结束，确认固件校验信息
#define CRC_CHECK_REQUEST (0x0205)
// payload信息
typedef struct
{
    uint16_t check_sum; // TODO: 暂时使用checksum
}__PACKED crc_check_req_t;
typedef struct
{
    ret_code_t ret_code;
}__PACKED crc_check_ack_t;

// class:0x02 id:0x07 APP运行，传输自身版本号信息
#define PUSH_VERSION_REQUEST (0x0207)
// payload信息
typedef struct
{
    version_info_t version;
}__PACKED push_version_req_t;
typedef struct
{
    ret_code_t ret_code;
}__PACKED push_version_ack_t;


#ifdef __cplusplus
}
#endif

#endif