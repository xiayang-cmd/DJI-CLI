#include "com_task.h"
#include <iostream>
#include <chrono>
#include <mutex>
#include <condition_variable>


// 使用全局队列：
extern std::queue<DataFrame> g_dataFrameQueue;
extern std::mutex g_queueMutex;
extern std::condition_variable g_queueCond;

ComTask::ComTask(CLinuxTCPCom& tcpCom)
    : m_tcpCom(tcpCom)
    , m_isRunning(true)  // 初始设为true，当socket关闭或其他方式时可令其退出
{
}


ComTask::~ComTask()
{
    // 退出时标记停止，若有需要也可以在这里强行 shutdown socket
    m_isRunning = false;
}

void ComTask::pushDataFrame(const DataFrame& frame)
{
    // 入队
    {
        std::lock_guard<std::mutex> lk(g_queueMutex);
        g_dataFrameQueue.push(frame);
    }
    // 唤醒发送线程
    g_queueCond.notify_one();
}

void ComTask::sendThreadFunc()
{
    std::cout << "[ComTask] sendThreadFunc started.\n";
    while (m_isRunning)
    {
        DataFrame frameToSend;
        {
            std::unique_lock<std::mutex> lk(g_queueMutex);
            // 等待队列非空或者任务被停止
            g_queueCond.wait(lk, [] {
                return !g_dataFrameQueue.empty();
            });

            // 再次判断是否仍在运行
            if (!m_isRunning) {
                break;
            }

            // 从队列中取出一帧
            frameToSend = g_dataFrameQueue.front();
            g_dataFrameQueue.pop();
        }

        // 调用 TCP 通信库发送数据
        int sentBytes = m_tcpCom.TCPSendData(frameToSend.data(), frameToSend.size());
        if (sentBytes > 0) {
            // std::cout << "[ComTask] Sent " << sentBytes << " bytes.\n";
        } else {
            std::cerr << "[ComTask] Send failed.\n";
        }

        // 小休眠，避免空转
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "[ComTask] sendThreadFunc exiting...\n";
}

void ComTask::recvThreadFunc()
{
    std::cout << "[ComTask] recvThreadFunc started.\n";
    uint8_t buf[1024] = {0};

    while (m_isRunning)
    {
        int received = m_tcpCom.TCPRecvData(buf, sizeof(buf));
        if (received > 0) {
            // // 简单打印
            // std::cout << "[ComTask] Received " << received << " bytes.\n";
            // for (int i = 0; i < received; i++)
            // {
            //     printf("%02X ", buf[i]);
            // }
            // printf("\n");

            // 构造 DataFrame
            DataFrame recvFrame(buf, buf + received); 
            {
                // 加锁并放入全局接收队列
                std::unique_lock<std::mutex> lk(g_recvRawQueueMutex);
                g_recvRawDataFrameQueue.push(std::move(recvFrame));
            }
            // 通知可能在等待数据的线程
            g_recvRawQueueCond.notify_one();
        }
        else if (received == 0) {
            // 对端关闭连接
            std::cerr << "[ComTask] Peer closed connection.\n";
            // 进入重连逻辑
            while(1){
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "[ComTask] Waiting for reconnection...\n";
                // 重新连接逻辑
                if (m_tcpCom.TCPInitClient("127.0.0.1", 12345) == 0) {
                    std::cout << "[ComTask] Reconnected successfully.\n";
                    break; // 成功重新连接后跳出循环
                } else {
                    std::cerr << "[ComTask] Reconnection failed, retrying...\n";
                }
            }
        }
        else {
            // 可能是出错，也可能是非阻塞模式下的暂时无数据
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // 小休眠，避免空转
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "[ComTask] recvThreadFunc exiting...\n";
}