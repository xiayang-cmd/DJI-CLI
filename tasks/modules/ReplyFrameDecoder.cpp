#include "ReplyFrameDecoder.h"

ReplyFrameDecoder::ReplyFrameDecoder()
{
    resetState();
}

void ReplyFrameDecoder::setDecodeCallback(std::function<void(uint8_t, const uint8_t*, uint16_t)> callback)
{
    decode_callback_ = callback;
}

void ReplyFrameDecoder::runDecodeThread()
{
    std::cout << "[ReplyFrameDecoder] runDecodeThread started.\n";

    while (true) {
        // 等待队列中有数据
        std::unique_lock<std::mutex> lock(g_completeQueueMutex);
        g_completeQueueCond.wait(lock, [] {
            return !g_completeDataFrameQueue.empty();
        });

        // 取出一帧数据
        DataFrame frame = g_completeDataFrameQueue.front();
        g_completeDataFrameQueue.pop();

        lock.unlock();

        // std::cout << "[ReplyFrameDecoder] Received a frame of size: " << frame.size() << std::endl;
        
        // 将该帧的每一个字节送入解析器
        read_state  = 0;
        for (uint8_t byte_data : frame) {
            processByte(byte_data);
        }
    }
}

void ReplyFrameDecoder::resetState()
{
    read_state  = 0;
    read_count  = 0;
    last_data   = 0;
    msg_length  = 0;
    payload_length = 0;
    src_data_length = 0;

    memset(msg_data, 0, RESP_MSG_LEN);
    memset(frame_header_bytes, 0, sizeof(frame_header_bytes));
    memset(payload_length_bytes, 0, sizeof(payload_length_bytes));
    memset(command_id_bytes, 0, sizeof(command_id_bytes));
    memset(src_data_bytes, 0, sizeof(src_data_bytes));
}

// --------------------- 解析各字段 -------------------------------------------
void ReplyFrameDecoder::processHeader(uint8_t data)
{
    // 示例：假设帧头是 0x6A 0x77
    if (last_data == 0x6A && data == 0x77) {
        read_state  = 1;
        msg_length  = 1;  // 帧头已检测到，外部调用后会自增
    } else {
        // 帧头未对齐, 继续等待
        read_state  = 0;
        msg_length  = 0;
    }
}

void ReplyFrameDecoder::processDataLength(uint8_t data)
{
    read_count++;
    payload_length_bytes[read_count - 1] = data;

    if (read_count == 2) {
        payload_length = (payload_length_bytes[0] << 8) | payload_length_bytes[1];
        // 指令编号占 1 字节, 剩余都是源数据
        src_data_length = (payload_length > 0) ? (payload_length - 1) : 0;
        src_data_length = std::min<uint16_t>(src_data_length, SRC_MAX_LEN);

        read_count = 0;
        read_state = 2;
    }
}

void ReplyFrameDecoder::processCommand(uint8_t data)
{
    command_id_bytes[0] = data;
    command_id = data;

    read_state = 3;
    read_count = 0;
}

void ReplyFrameDecoder::processSrcData(uint8_t data)
{
    if (read_count < SRC_MAX_LEN) {
        src_data_bytes[read_count] = data;
    }
    read_count++;

    // 判断是否达到源数据长度
    if (read_count == src_data_length) {
        // 一帧解析完毕, 可以在这里触发回调或处理数据
        // -----------------------------------------
        // 如果设置了回调函数，则将解析结果抛给外部
        if (decode_callback_) {
            decode_callback_(command_id, src_data_bytes, src_data_length);
        }
        // -----------------------------------------

        // 重置
        read_state = 0;
        read_count = 0;
    }
}

// ---------------------- 状态机入口 ------------------------------------------
void ReplyFrameDecoder::processByte(uint8_t byte_data)
{
    switch (read_state) {
    case 0:
        processHeader(byte_data);
        break;
    case 1:
        processDataLength(byte_data);
        break;
    case 2:
        processCommand(byte_data);
        break;
    case 3:
        processSrcData(byte_data);
        break;
    default:
        resetState();
        break;
    }

    // 保存公共信息
    last_data = byte_data;

    if (msg_length < RESP_MSG_LEN) {
        msg_data[msg_length] = byte_data;
    }
    msg_length++;
}

// ----------------------- Debug 打印 -----------------------------------------
void ReplyFrameDecoder::printMsgData()
{
    std::cout << "回复指令数据: ";
    for (uint16_t i = 0; i < msg_length; ++i) {
        printf("%02X ", msg_data[i]);
    }
    std::cout << std::endl;
}
