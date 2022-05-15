#include "bin_transfer_protocol.h"
#include "link_mac.h"
#include "uart_port.h"
#include <stdlib.h>
#include <string.h>

typedef enum{
    CHECK_VERSION,
    INTO_UPGRADE_REQ,
    INTO_LOADER_REQ,
    GET_TRANSFER_CONFIG_ACK,
    BIN_TRANSFER,
    CRC_CHECK_REQ,
    WAIT_APP_VERSION_REQ,
}bin_transfer_step_t;

bin_transfer_step_t cur_step = CHECK_VERSION;

#define STM32_APP_BIN_ONE_PACKAGE_MAX_SIZE 512

#define TIMEOUT_TRANSFER (2000)

ret_t wait_ack(uint16_t msg_class_id, uint32_t timeout)
{
    // TODO: 从串口接收函数实现等ack的逻辑
    return RET_SUCCESS;
}


void bin_transfer_entry(uint8_t *bin_buffer, uint32_t bin_size)
{
    uint16_t    frame_buffer_size  = STM32_APP_BIN_ONE_PACKAGE_MAX_SIZE + sizeof(data_transfer_req_t) + sizeof(mac_frame_t);
    mac_frame_t *frame_buffer      = (mac_frame_t*)malloc(frame_buffer_size);
    uint16_t    transfer_data_size = 0;
    // 查询版本号
    cur_step = CHECK_VERSION;
    get_version_req_t _get_version;
    _get_version.reserved = 0xFF;
    transfer_data_size    = frame_buffer_size;
    pack_one_frame(GET_VERSION_REQUEST, (uint8_t *)&_get_version, sizeof(_get_version), frame_buffer, &transfer_data_size);
    transfer_data_to_port_sync(0, (uint8_t*)frame_buffer, &transfer_data_size, TIMEOUT_TRANSFER);
    wait_ack(GET_VERSION_REQUEST, TIMEOUT_TRANSFER);

    // 请求开始进行固件升级
    cur_step = INTO_UPGRADE_REQ;
    memset((void *)frame_buffer, 0, frame_buffer_size);
    into_upgrade_req_t _into_upgrade_req;
    _into_upgrade_req.reserved = 0xFF;
    transfer_data_size    = frame_buffer_size;
    pack_one_frame(REQUEST_INTO_UPGRADE, (uint8_t *)&_into_upgrade_req, sizeof(_into_upgrade_req), frame_buffer, &transfer_data_size);
    transfer_data_to_port_sync(0, (uint8_t*)frame_buffer, &transfer_data_size, TIMEOUT_TRANSFER);
    wait_ack(REQUEST_INTO_UPGRADE, TIMEOUT_TRANSFER);

    // 请求进入loader
    cur_step = INTO_LOADER_REQ;
    memset(frame_buffer, 0, frame_buffer_size);
    into_loader_req_t _into_loader_req;
    _into_loader_req.reserved = 0xFF;
    transfer_data_size    = frame_buffer_size;
    pack_one_frame(REQUEST_INTO_LOADER, (uint8_t *)&_into_loader_req, sizeof(_into_loader_req), frame_buffer, &transfer_data_size);
    transfer_data_to_port_sync(0, (uint8_t*)frame_buffer, &transfer_data_size, TIMEOUT_TRANSFER);
    wait_ack(REQUEST_INTO_LOADER, TIMEOUT_TRANSFER);

    // TODO: 从这里拿到最大传输长度和单个固件切片的传输长度
    cur_step = GET_TRANSFER_CONFIG_ACK;

    wait_ack(REQUEST_TRANSFER_CONFIG, TIMEOUT_TRANSFER);

    uint32_t one_package_size              = 512;
    uint32_t max_bin_size                  = 1024*1024;
    uint32_t max_one_package_write_timeout = 200;

    uint8_t             is_transfer_done           = 0;
    uint32_t            cur_write_package_index    = 0;
                        cur_step                   = BIN_TRANSFER;
    data_transfer_req_t *_data_transfer_req        = (data_transfer_req_t*)malloc(STM32_APP_BIN_ONE_PACKAGE_MAX_SIZE + sizeof(data_transfer_req_t));
    uint32_t            cur_file_buffer_read_index = 0;
    uint32_t            max_package_write_times    = bin_size/one_package_size + 1;
    DBG("bin size:%d, need write times:%d...\n", bin_size, max_package_write_times);
    while(!is_transfer_done){
        DBG("cur package_index:%d ..\n", cur_write_package_index);
        // 固件传输
        memset(frame_buffer, 0, frame_buffer_size);
        memset(_data_transfer_req, 0, STM32_APP_BIN_ONE_PACKAGE_MAX_SIZE + sizeof(data_transfer_req_t));

        cur_file_buffer_read_index = cur_write_package_index*one_package_size;
        _data_transfer_req->cur_package_index = cur_file_buffer_read_index;

        // 还没到最后一次
        if((cur_write_package_index+1) < max_package_write_times){
            _data_transfer_req->package_size = one_package_size;
        }else{
            _data_transfer_req->package_size = bin_size % one_package_size;
            DBG("reach last package .. size:%d..\n", _data_transfer_req->package_size);
            is_transfer_done = 1;
        }
        memcpy((void *)_data_transfer_req->package_buffer,
                (void *)&bin_buffer[cur_file_buffer_read_index],
                _data_transfer_req->package_size);

        transfer_data_size    = frame_buffer_size;
        pack_one_frame(DATA_TRANSFER_REQUEST, (uint8_t *)_data_transfer_req, sizeof(_data_transfer_req)+_data_transfer_req->package_size, frame_buffer, &transfer_data_size);
        transfer_data_to_port_sync(0, (uint8_t*)frame_buffer, &transfer_data_size, TIMEOUT_TRANSFER);
        wait_ack(DATA_TRANSFER_REQUEST, TIMEOUT_TRANSFER);

        cur_write_package_index++;
    }

}


