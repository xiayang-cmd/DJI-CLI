#pragma once

#include <vector>
#include <atomic>
#include "common_types.h"

// /**
//  * @brief 全局使用的数据帧定义
//  */
// using DataFrame = std::vector<uint8_t>;

/**
 * @brief FrameAssembler 类：
 *        从全局的原始数据帧队列 g_recvRawDataFrameQueue 中提取数据，
 *        解析成完整帧后放入 g_completeDataFrameQueue
 *
 *        - 当 discardMode = true 时（舍弃模式）：
 *            1. 如果当前缓冲里数据连 0.5 帧都不够，直接丢弃。
 *            2. 如果能拼出 1 帧后，将后续剩余数据全部丢弃。
 *        - 当 discardMode = false 时（正常模式）：
 *            1. 循环缓冲方式，尽量从已接收的数据中解析出所有完整帧。
 *            2. 不足一帧的部分保留在缓冲区等待下次数据继续拼接。
 *
 *        使用方法：
 *            1. 在主线程中构造 FrameAssembler 对象。
 *            2. 在外部创建线程（std::thread）调用对象的 run() 函数。
 *            3. 在需要停止时调用 stop()，并等待该线程退出。
 */
class FrameAssembler
{
    // 让 TasksManager 可以直接调用本类的私有成员（包括线程函数）
    friend class TasksManager;
    
public:
    /**
     * @param discardMode 是否开启“舍弃模式”
     */
    explicit FrameAssembler(bool discardMode = false);

    /**
     * @brief 析构函数
     */
    ~FrameAssembler();

    /**
     * @brief 在外部线程中执行此函数，循环从全局队列中取数据并解析成完整帧
     *        直到 stop() 被调用或对象被析构（m_stopFlag = true）。
     */
    void run();

    /**
     * @brief 设置停止标志，退出 run() 的循环
     */
    void stop();

private:
    /**
     * @brief 从内部缓冲 m_buffer 中解析完整帧逻辑
     *        可能一次解析出多帧（正常模式），也可能只解析一帧后丢弃后续（舍弃模式）
     */
    void parseBuffer();

private:
    bool m_discardMode;             ///< 是否为舍弃模式
    std::atomic<bool> m_stopFlag;   ///< 停止标志
    std::vector<uint8_t> m_buffer;  ///< 用于拼接、解析数据帧的临时缓冲
};

