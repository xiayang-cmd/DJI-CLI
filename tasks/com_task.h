#pragma once

#include <thread>
#include <atomic>
#include <condition_variable>
#include "utils/CLinuxTCPCom.h"
#include "common_types.h"

/**
 * @brief 通信任务类，负责：
 *  1. 从队列中取数据帧并发送给服务器
 *  2. 接收服务器数据并做相应处理（此处仅示例打印/解析）
 *
 * ComTask 仅关注“做什么”，不管理线程的生命周期，线程管理由 TasksManager 负责。
 */
class ComTask
{
    // 让 TasksManager 可以直接调用本类的私有成员（包括线程函数）
    friend class TasksManager;

public:
    /**
     * @param tcpCom 外部传入的TCP通信对象引用，用于发送/接收数据
     */
    explicit ComTask(CLinuxTCPCom& tcpCom);

    /**
     * @brief 析构函数
     */
    ~ComTask();

    /**
     * @brief 向队列中推送数据帧
     */
    void pushDataFrame(const DataFrame& frame);

private:
    /**
     * @brief 发送线程函数：不断从队列中获取数据帧并发送
     */
    void sendThreadFunc();

    /**
     * @brief 接收线程函数：不断接收服务器数据并简单打印
     */
    void recvThreadFunc();

private:
    CLinuxTCPCom&      m_tcpCom;     ///< 引用外部的TCP通信实例
    std::atomic<bool>  m_isRunning;  ///< 控制任务是否继续运行
};
