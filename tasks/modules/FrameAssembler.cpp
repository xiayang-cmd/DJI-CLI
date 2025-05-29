#include "FrameAssembler.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

// // ----------------------------
// // 声明全局队列与同步变量（你可能在别的地方有相同声明，也可放到统一的头文件）
// // ----------------------------
// extern std::queue<DataFrame> g_recvRawDataFrameQueue;
// extern std::mutex g_recvRawQueueMutex;
// extern std::condition_variable g_recvRawQueueCond;

// extern std::queue<DataFrame> g_completeDataFrameQueue;
// extern std::mutex g_completeQueueMutex;
// extern std::condition_variable g_completeQueueCond;


// -----------------------------------------------------------------
// 构造 / 析构
// -----------------------------------------------------------------
FrameAssembler::FrameAssembler(bool discardMode)
    : m_discardMode(discardMode)
    , m_stopFlag(false)
{
}

FrameAssembler::~FrameAssembler()
{
    // 如果还有可能在 run() 中等待，需要确保不会阻塞析构。
    // 一般情况下，建议在外部显式调用 stop() 再 join 线程。
    // 这里留空即可。
}

// -----------------------------------------------------------------
// 停止函数：只设置标志并唤醒可能的等待
// -----------------------------------------------------------------
void FrameAssembler::stop()
{
    m_stopFlag.store(true);
    // 唤醒等待 g_recvRawQueueCond 的线程，避免卡住
    g_recvRawQueueCond.notify_all();
}

// -----------------------------------------------------------------
// 线程体：循环从 g_recvRawDataFrameQueue 中取数据帧，追加到缓冲并解析
// -----------------------------------------------------------------
void FrameAssembler::run()
{
    std::cout << "[FrameAssembler] run() started.\n";

    while (!m_stopFlag.load())
    {
        // 等待 原始数据队列 有新数据或停止
        std::unique_lock<std::mutex> lock(g_recvRawQueueMutex);
        g_recvRawQueueCond.wait(lock, [&]{
            return !g_recvRawDataFrameQueue.empty() || m_stopFlag.load();
        });

        // 再次检查停止标志
        if (m_stopFlag.load()) {
            break;
        }

        // 从队列中取出一个原始数据帧
        DataFrame rawFrame = g_recvRawDataFrameQueue.front();
        g_recvRawDataFrameQueue.pop();
        lock.unlock();

        // std::cout << "[FrameAssembler] Received raw frame of size: " << rawFrame.size() << "\n";

        // 追加到本地缓冲
        m_buffer.insert(m_buffer.end(), rawFrame.begin(), rawFrame.end());

        // 尝试解析缓冲区
        parseBuffer();
    }
}

// -----------------------------------------------------------------
// 解析缓冲区，组装完整帧并推入 g_completeDataFrameQueue
// -----------------------------------------------------------------
void FrameAssembler::parseBuffer()
{
    while (true)
    {
        // 1. 检查最小长度（至少要有 4 字节：帧头 2 + 长度 2 才能继续）
        if (m_buffer.size() < 4)
        {
            // 舍弃模式下，如果连 3 字节都不到，就直接丢弃（示例逻辑，可自行调整）
            if (m_discardMode && m_buffer.size() < 3) {
                m_buffer.clear();
            }
            break;
        }

        // 2. 检查帧头是否 0x6A 0x77
        if (m_buffer[0] != 0x6A || m_buffer[1] != 0x77)
        {
            // 帧头不对，丢弃一个字节，继续查找
            m_buffer.erase(m_buffer.begin());
            continue;
        }

        // 3. 读取长度字段(第 2、3 字节)，假设是大端
        uint16_t length = (static_cast<uint16_t>(m_buffer[2]) << 8) | m_buffer[3];
        // 完整帧总大小 = 帧头 2 + length 字节数 2 + 数据 length
        size_t frameSize = 4 + length;

        // 4. 判断当前缓冲是否足以得到一个完整帧
        if (m_buffer.size() < frameSize)
        {
            // 数据还不够一帧
            if (m_discardMode)
            {
                // 若小于半帧直接丢弃（这里用 frameSize/2.0 示例）
                if (m_buffer.size() < (frameSize / 2.0)) {
                    m_buffer.clear();
                }
            }
            // 不足一帧，先留着等待下次来更多数据再拼
            break;
        }

        // 5. 构造一个完整帧
        DataFrame completeFrame(m_buffer.begin(), m_buffer.begin() + frameSize);

        // 6. 放入完整帧队列
        {
            std::lock_guard<std::mutex> lk(g_completeQueueMutex);
            g_completeDataFrameQueue.push(std::move(completeFrame));
        }
        g_completeQueueCond.notify_one();

        // 7. 从缓冲区移除已解析好的帧数据
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + frameSize);

        // 8. 舍弃模式：只要解析出一帧，就丢弃剩余并退出循环
        if (m_discardMode)
        {
            m_buffer.clear();
            break;
        }
        // 非舍弃模式：继续循环，可能还会有下一帧
    }
}
