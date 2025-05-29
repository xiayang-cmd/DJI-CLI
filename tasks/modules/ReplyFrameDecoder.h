#ifndef REPLY_FRAME_DECODER_H
#define REPLY_FRAME_DECODER_H

#include <cstdint>        // uint8_t, uint16_t
#include <cstring>        // memset, memcpy
#include <cstdio>         // printf
#include <iostream>       // std::cout
#include <algorithm>      // std::min
#include <functional>     // std::function

#include "common_types.h" // 这里可以包含 extern 声明的全局队列、互斥量等

// ----------------------------------------------------------------------------
// 回复数据帧格式
//    1. 帧头      : 0x74 0x79 (2 字节)
//    2. 数据长度  : uint16_t, 包括「指令编号 + 源数据」长度 (2 字节, 高位在前)
//    3. 指令编号  : 1 字节
//    4. 源数据    : 0 ~ (SRC_MAX_LEN-1) 字节
// ----------------------------------------------------------------------------

#define SRC_MAX_LEN   512          // 源数据最大长度
#define RESP_MSG_LEN  1024         // 单帧最大缓存

class ReplyFrameDecoder 
{
    // 让 TasksManager 可以直接调用本类的私有成员（包括线程函数）
    friend class TasksManager;

public:
    ReplyFrameDecoder();

    /**
     * @brief 设置解析完成回调。当一帧完整解析完毕后，会调用此回调。
     * @param callback  回调函数，参数依次为:
     *                  - command_id:  指令编号
     *                  - src_data:    源数据指针
     *                  - src_length:  源数据长度
     */
    void setDecodeCallback(std::function<void(uint8_t, const uint8_t*, uint16_t)> callback);

    /**
     * @brief 线程函数（**不在内部创建线程**），调用者在外部自行开启线程执行本函数。
     *        该函数会阻塞等待队列中出现新的数据帧，并将其逐字节送入状态机进行解析。
     */
    void runDecodeThread();

    // 打印已接收的整个帧，仅作调试用
    void printMsgData();

private:
    // 逐字节推进状态机
    void processByte(uint8_t byte_data);

    // 解析器重置
    void resetState();

    // 以下是具体的解析步骤
    void processHeader(uint8_t data);
    void processDataLength(uint8_t data);
    void processCommand(uint8_t data);
    void processSrcData(uint8_t data);

private:
    // 原始帧缓存
    uint8_t  msg_data[RESP_MSG_LEN];
    uint16_t msg_length;

    // 1. 帧头
    uint8_t  frame_header_bytes[2];
    uint16_t frame_header;

    // 2. 数据长度
    uint8_t  payload_length_bytes[2];
    uint16_t payload_length;

    // 3. 指令编号
    uint8_t  command_id_bytes[1];
    uint8_t  command_id;

    // 4. 源数据
    uint8_t  src_data_bytes[SRC_MAX_LEN];
    uint16_t src_data_length;

    // 解析状态机
    uint8_t  read_state;
    uint16_t read_count;
    uint8_t  last_data;

    // 完整解析后回调函数
    std::function<void(uint8_t, const uint8_t*, uint16_t)> decode_callback_;
};

#endif // REPLY_FRAME_DECODER_H
