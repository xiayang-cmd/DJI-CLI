#pragma once

#include <vector>
#include <cstdint>
#include <queue>
#include <mutex>
#include <condition_variable>

// 定义服务器配置结构体
struct ServerConfig {
    std::string ip;
    int         port;
    bool        is_valid = false; // 是否有效的配置
};

// 数据帧类型
using DataFrame = std::vector<uint8_t>;

/**
 * @brief 全局队列，用于存放待发送的数据帧
 */
extern std::queue<DataFrame> g_dataFrameQueue;
extern std::mutex g_queueMutex;
extern std::condition_variable g_queueCond;

/**
 * @brief 全局队列，用于存放接收到的原始数据帧
 */
extern std::queue<DataFrame> g_recvRawDataFrameQueue;
extern std::mutex g_recvRawQueueMutex;
extern std::condition_variable g_recvRawQueueCond;

/**
 * @brief 全局队列，用于存放封装到的完整数据帧
 */
extern std::queue<DataFrame> g_completeDataFrameQueue;
extern std::mutex g_completeQueueMutex;
extern std::condition_variable g_completeQueueCond;

extern ServerConfig g_serverConfig; // 全局服务器配置



