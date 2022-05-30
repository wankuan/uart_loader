#include "bin_transfer_protocol.h"
#include "link_mac.h"
#include "bin_transfer.hpp"
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include "crc32.h"
#include <pthread.h>
#include <errno.h>

#include <condition_variable>
#include <mutex>

std::mutex              wait_lock;
std::condition_variable cv;

uint32_t cur_tansfer_index = (int32_t)-1;

typedef enum {
    INIT,
    WAIT_THREAD_READY,
    CHECK_VERSION,
    INTO_UPGRADE_REQ,
    INTO_LOADER_REQ,
    GET_TRANSFER_CONFIG_ACK,
    BIN_TRANSFER,
    CRC_CHECK_REQ,
    WAIT_APP_VERSION_REQ,
} bin_transfer_step_t;

bin_transfer_step_t cur_step = INIT;

#define STM32_APP_BIN_ONE_PACKAGE_MAX_SIZE 512

#define TIMEOUT_TRANSFER (2000)

void *unpack_frame_thread(void *args)
{
    printf("\n +----------------------------------+");
    printf("\n |       unpack_frame_thread        |");
    printf("\n +----------------------------------+");
    printf("\n");

    UartCore *uart_device = reinterpret_cast<UartCore *>(args);
    uint32_t  read_size   = 0;
    uint8_t   read_buffer[MAX_FRAME_PAYLOAD_LENGTH + 10];

    {
        std::lock_guard<std::mutex> lk(wait_lock);
        cur_step = WAIT_THREAD_READY;
    }
    cv.notify_all();
    // INFO("start unpack..\n");
    while (1) {
        read_size = MAX_FRAME_PAYLOAD_LENGTH + 10;
        read_size = uart_device->ReadData(read_buffer, read_size);
        DBG("[READ MONITOR] read_size:%d\n", read_size);

        unpack_stream_cycle(read_buffer, read_size);

        // 1ms loop
        usleep(1000);
    }
}

ret_t ack_get_version(mac_frame_t *frame)
{
    if (frame == NULL) {
        return RET_FAIL;
    }
    if (frame->type != ACK) {
        ERROR("msg_class:0x%x msg_id:0x%x is not a ack..\n", frame->msg_class, frame->msg_id);
        return RET_FAIL;
    }
    {
        std::lock_guard<std::mutex> lk(wait_lock);
        cur_step = CHECK_VERSION;
    }
    cv.notify_all();
    get_version_ack_t *_ack = reinterpret_cast<get_version_ack_t *>(frame->payload);
    INFO("version..%.2x.%.2x.%.2x.%.2x   str:%s\n", _ack->version.AA, _ack->version.BB, _ack->version.CC, _ack->version.DD,
         _ack->version.version_str);
    return RET_SUCCESS;
}

ret_t ack_into_request_into_upgrade(mac_frame_t *frame)
{
    if (frame == NULL) {
        return RET_FAIL;
    }
    if (frame->type != ACK) {
        ERROR("msg_class:0x%x msg_id:0x%x is not a ack..\n", frame->msg_class, frame->msg_id);
        return RET_FAIL;
    }
    INFO("ack -> %s\n", __func__);
    {
        std::lock_guard<std::mutex> lk(wait_lock);
        cur_step = INTO_UPGRADE_REQ;
    }
    cv.notify_all();
    return RET_SUCCESS;
}
ret_t ack_into_request_into_loader(mac_frame_t *frame)
{
    if (frame == NULL) {
        return RET_FAIL;
    }
    if (frame->type != ACK) {
        ERROR("msg_class:0x%x msg_id:0x%x is not a ack..\n", frame->msg_class, frame->msg_id);
        return RET_FAIL;
    }
    INFO("ack -> %s\n", __func__);
    {
        std::lock_guard<std::mutex> lk(wait_lock);
        cur_step = INTO_LOADER_REQ;
    }
    cv.notify_all();
    return RET_SUCCESS;
}

uint32_t loader_bin_max_size;
uint32_t one_package_size;
uint32_t max_one_package_write_timeout;
ret_t    request_get_transer_config(mac_frame_t *frame)
{
    if (frame == NULL) {
        return RET_FAIL;
    }
    if (frame->type != REQUEST) {
        ERROR("msg_class:0x%x msg_id:0x%x is not a REQUEST..\n", frame->msg_class, frame->msg_id);
        return RET_FAIL;
    }
    INFO("request -> %s\n", __func__);
    transfer_config_req_t *_req   = reinterpret_cast<transfer_config_req_t *>(frame->payload);
    loader_bin_max_size           = _req->max_bin_size;
    one_package_size              = _req->one_package_size;
    max_one_package_write_timeout = _req->max_one_package_write_timeout;
    {
        std::lock_guard<std::mutex> lk(wait_lock);
        cur_step = GET_TRANSFER_CONFIG_ACK;
    }
    cv.notify_all();
    return RET_SUCCESS;
}

ret_t ack_data_transfer_request(mac_frame_t *frame)
{
    if (frame == NULL) {
        return RET_FAIL;
    }
    if (frame->type != ACK) {
        ERROR("msg_class:0x%x msg_id:0x%x is not a ack..\n", frame->msg_class, frame->msg_id);
        return RET_FAIL;
    }
    data_transfer_ack_t *_ack = reinterpret_cast<data_transfer_ack_t *>(frame->payload);
    if (_ack->ret_code != RET_SUCCESS) {
        // TODO: 发起重传
        ERROR("[transfer bin] index:%d ack is fail..\n", _ack->next_package_index);
        return RET_FAIL;
    }
    INFO("bin transfer ack.. next_index:%d\n", _ack->next_package_index);
    {
        std::lock_guard<std::mutex> lk(wait_lock);
        cur_tansfer_index = _ack->next_package_index;
    }
    cv.notify_all();
    return RET_SUCCESS;
}
ret_t ack_crc_check_request(mac_frame_t *frame)
{
    if (frame == NULL) {
        return RET_FAIL;
    }
    if (frame->type != ACK) {
        ERROR("msg_class:0x%x msg_id:0x%x is not a ack..\n", frame->msg_class, frame->msg_id);
        return RET_FAIL;
    }
    {
        std::lock_guard<std::mutex> lk(wait_lock);
        cur_step = CRC_CHECK_REQ;
    }
    cv.notify_all();
    return RET_SUCCESS;
}
void register_link_callback()
{
    register_frame_callback(GET_VERSION_REQUEST, ack_get_version);
    register_frame_callback(REQUEST_INTO_UPGRADE, ack_into_request_into_upgrade);
    register_frame_callback(REQUEST_INTO_LOADER, ack_into_request_into_loader);
    register_frame_callback(REQUEST_TRANSFER_CONFIG, request_get_transer_config);
    register_frame_callback(DATA_TRANSFER_REQUEST, ack_data_transfer_request);
    register_frame_callback(CRC_CHECK_REQUEST, ack_crc_check_request);
}
ret_t wait_transfer_ack(uint32_t wait_index, uint32_t timeout)
{
    {
        std::unique_lock<std::mutex> lk(wait_lock);
        cv.wait(lk, [wait_index] { return cur_tansfer_index == wait_index; });
    }
    return RET_SUCCESS;
}

ret_t wait_ack(uint16_t msg_class_id, uint32_t timeout)
{
    // 当wait_step等于cur_step时退出
    bin_transfer_step_t wait_step;
    switch (msg_class_id) {
        case GET_VERSION_REQUEST:
            wait_step = CHECK_VERSION;
            break;
        case REQUEST_INTO_UPGRADE:
            wait_step = INTO_UPGRADE_REQ;
            break;
        case REQUEST_INTO_LOADER:
            wait_step = INTO_LOADER_REQ;
            break;
        case REQUEST_TRANSFER_CONFIG:
            wait_step = GET_TRANSFER_CONFIG_ACK;
            break;
        case DATA_TRANSFER_REQUEST:
            wait_step = BIN_TRANSFER;
            break;
        case CRC_CHECK_REQUEST:
            wait_step = CRC_CHECK_REQ;
            break;
        case PUSH_VERSION_REQUEST:
            wait_step = WAIT_APP_VERSION_REQ;
            break;
        default:
            break;
    }
    {
        std::unique_lock<std::mutex> lk(wait_lock);
        cv.wait(lk, [wait_step] { return cur_step == wait_step; });
    }
    return RET_SUCCESS;
}

void bin_transfer_entry(UartCore &uart_device, uint8_t *bin_buffer, uint32_t bin_size)
{
    uint16_t     frame_buffer_size  = STM32_APP_BIN_ONE_PACKAGE_MAX_SIZE + sizeof(data_transfer_req_t) + sizeof(mac_frame_t);
    mac_frame_t *frame_buffer       = (mac_frame_t *)malloc(frame_buffer_size);
    uint16_t     transfer_data_size = 0;

    // 创建解包线程
    int       err;
    pthread_t ntid;
    err = pthread_create(&ntid, NULL, unpack_frame_thread, &uart_device);
    if (err != 0) {
        printf("can't create thread: %s\n", strerror(err));
    }
    // 初始化解包回调
    frame_callback_table_init();
    register_link_callback();

    INFO("wait unpack thread ready..\n");
    // 等待解包线程ready
    {
        std::unique_lock<std::mutex> lk(wait_lock);
        cv.wait(lk, [] { return cur_step == WAIT_THREAD_READY; });
    }
    // 查询版本号
    INFO("send GET_VERSION_REQUEST..\n");
    get_version_req_t _get_version;
    _get_version.reserved = 0xFF;
    transfer_data_size    = frame_buffer_size;
    pack_one_frame(GET_VERSION_REQUEST, REQUEST, (uint8_t *)&_get_version, sizeof(_get_version), frame_buffer, &transfer_data_size);
    transfer_data_size = uart_device.SendData((uint8_t *)frame_buffer, transfer_data_size);
    // transfer_data_to_port_sync(0, (uint8_t*)frame_buffer, &transfer_data_size, TIMEOUT_TRANSFER);
    wait_ack(GET_VERSION_REQUEST, TIMEOUT_TRANSFER);

    // 请求开始进行固件升级
    INFO("send INTO_UPGRADE_REQ..\n");
    memset((void *)frame_buffer, 0, frame_buffer_size);
    into_upgrade_req_t _into_upgrade_req;
    _into_upgrade_req.reserved = 0xFF;
    transfer_data_size         = frame_buffer_size;
    pack_one_frame(REQUEST_INTO_UPGRADE, REQUEST, (uint8_t *)&_into_upgrade_req, sizeof(_into_upgrade_req), frame_buffer, &transfer_data_size);
    transfer_data_size = uart_device.SendData((uint8_t *)frame_buffer, transfer_data_size);
    wait_ack(REQUEST_INTO_UPGRADE, TIMEOUT_TRANSFER);

    // 请求进入loader
    INFO("send INTO_LOADER_REQ..\n");
    memset(frame_buffer, 0, frame_buffer_size);
    into_loader_req_t _into_loader_req;
    _into_loader_req.reserved = 0xFF;
    transfer_data_size        = frame_buffer_size;
    pack_one_frame(REQUEST_INTO_LOADER, REQUEST, (uint8_t *)&_into_loader_req, sizeof(_into_loader_req), frame_buffer, &transfer_data_size);
    transfer_data_size = uart_device.SendData((uint8_t *)frame_buffer, transfer_data_size);
    wait_ack(REQUEST_INTO_LOADER, TIMEOUT_TRANSFER);

    INFO("wait REQUEST_TRANSFER_CONFIG request..\n");
    wait_ack(REQUEST_TRANSFER_CONFIG, TIMEOUT_TRANSFER);

    uint32_t max_package_write_times = bin_size / one_package_size + 1;

    transfer_config_ack_t _ack;
    _ack.ret_code         = RET_SUCCESS;
    _ack.bin_total_size   = bin_size;
    _ack.one_package_size = one_package_size;
    _ack.package_nums     = max_package_write_times;
    transfer_data_size    = frame_buffer_size;
    pack_one_frame(REQUEST_TRANSFER_CONFIG, ACK, (uint8_t *)&_ack, sizeof(_ack), frame_buffer, &transfer_data_size);
    transfer_data_size = uart_device.SendData((uint8_t *)frame_buffer, transfer_data_size);

    INFO("[BIN TRANSFER CONFIG] bin_size:%d, one_package_size:%d, need write times:%d ...\n", bin_size, one_package_size, max_package_write_times);

    uint32_t max_bin_size                  = 1024 * 1024;
    uint32_t max_one_package_write_timeout = 200;

    uint8_t  is_transfer_done                       = 0;
    uint32_t cur_write_package_index                = 0;
    cur_step                                        = BIN_TRANSFER;
    data_transfer_req_t *_data_transfer_req         = (data_transfer_req_t *)malloc(STM32_APP_BIN_ONE_PACKAGE_MAX_SIZE + sizeof(data_transfer_req_t));
    uint32_t             cur_file_buffer_read_index = 0;
    while (!is_transfer_done) {
        DBG("cur package_index:%d ..\n", cur_write_package_index);
        // 固件传输
        memset(frame_buffer, 0, frame_buffer_size);
        memset(_data_transfer_req, 0, STM32_APP_BIN_ONE_PACKAGE_MAX_SIZE + sizeof(data_transfer_req_t));

        cur_file_buffer_read_index            = cur_write_package_index * one_package_size;
        _data_transfer_req->cur_package_index = cur_write_package_index;

        // 还没到最后一次
        if ((cur_write_package_index + 1) < max_package_write_times) {
            _data_transfer_req->package_size = one_package_size;
        } else {
            _data_transfer_req->package_size = bin_size % one_package_size;
            INFO("last package:%d .. size:%d..\n", cur_write_package_index, _data_transfer_req->package_size);
            is_transfer_done = 1;
        }
        memcpy((void *)_data_transfer_req->package_buffer, (void *)&bin_buffer[cur_file_buffer_read_index], _data_transfer_req->package_size);

        transfer_data_size         = frame_buffer_size;
        static uint32_t crc_result = 0xFFFFFFFF;
        crc_result                 = crc32_cal((uint8_t *)_data_transfer_req->package_buffer, _data_transfer_req->package_size, crc_result);
        pack_one_frame(DATA_TRANSFER_REQUEST, REQUEST, (uint8_t *)_data_transfer_req, sizeof(_data_transfer_req) + _data_transfer_req->package_size,
                       frame_buffer, &transfer_data_size);

        INFO("send DATA_TRANSFER_REQUEST.. crc:0x%x  index:%d  process:%.2f%%\n", crc_result, cur_write_package_index,
             (float)(cur_write_package_index + 1) / max_package_write_times * 100);

        transfer_data_size = uart_device.SendData((uint8_t *)frame_buffer, transfer_data_size);
        // transfer_data_to_port_sync(0, (uint8_t*)frame_buffer, &transfer_data_size, TIMEOUT_TRANSFER);
        wait_transfer_ack(cur_write_package_index + 1, TIMEOUT_TRANSFER);
        cur_write_package_index++;
        // usleep(20 * 1000);
    }
}
