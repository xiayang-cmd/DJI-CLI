#include "TasksManager.h"
#include <iostream>
#include <chrono>
#include <thread>

TasksManager::TasksManager()
    : mIsRunning(false)
{
}

TasksManager::~TasksManager()
{
    stopAllTasks();
}

bool TasksManager::initAllTasks()
{
    std::cout << "[TasksManager] Init all tasks...\n";

    // 0. 获取服务器配置
    if(g_serverConfig.is_valid) {
        std::cout << "[TasksManager] Server config: " << g_serverConfig.ip << ":" << g_serverConfig.port << "\n";
    } else {
        std::cerr << "[TasksManager] Invalid server config!\n";
        return false; // 如果配置无效，直接返回
    }

    // 1. 初始化TCP通信对象（client模式举例）
    mTcpCom = std::make_unique<CLinuxTCPCom>();
    while (true) {
        if (mTcpCom->TCPInitClient(g_serverConfig.ip.c_str(), g_serverConfig.port) >= 0) {
            std::cout << "[TasksManager] TCP client initialized successfully.\n";
            break;
        }
        std::cerr << "[TasksManager] Failed to initialize TCP client, retrying...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }


    // 2. 创建ComTask对象
    mComTask = std::make_unique<ComTask>(*mTcpCom);

    // 3. 创建帧组装器
    mFrameAssembler = std::make_unique<FrameAssembler>(false); // false表示正常模式

    // 4. 创建回复帧解析器
    mReplyDecoder = std::make_unique<ReplyFrameDecoder>();

    // 创建 FrameDataHandler 实例（使用智能指针，便于管理）
    auto frameDataHandler = std::make_shared<FrameDataHandler>();

    // 设置回调函数，交给 FrameDataHandler 处理
    mReplyDecoder->setDecodeCallback(
        [frameDataHandler](uint8_t cmdId, const uint8_t* data, uint16_t length)
        {
            frameDataHandler->handleFrameData(cmdId, data, length);
        }
    );

    return true; // 初始化成功

}

void TasksManager::startAllTasks()
{
    if (mIsRunning)
    {
        std::cout << "[TasksManager] Tasks are already running." << std::endl;
        return;
    }

    mIsRunning = true;
    std::cout << "[TasksManager] Starting all tasks..." << std::endl;

    // 启动发送线程
    mThreads.emplace_back(&TasksManager::sendTaskFunc, this);

    // 启动接收线程
    mThreads.emplace_back(&TasksManager::recvTaskFunc, this);

    // 启动取帧线程
    mThreads.emplace_back(&TasksManager::assembleCompleteFrame, this);

    // 启动解析回复帧线程
    mThreads.emplace_back(&TasksManager::decodeReplyFrame, this);
}

void TasksManager::stopAllTasks()
{
    if (!mIsRunning) {
        std::cout << "[TasksManager] Tasks are already stopped.\n";
        return;
    }
    mIsRunning = false;

    std::cout << "[TasksManager] Stopping all tasks...\n";

    // 关闭socket，促使recv()返回，进而退出recv线程循环
    if (mTcpCom) {
        mTcpCom->CloseFd();
    }

    mComTask->m_isRunning = false;  // 停止通信任务

    mFrameAssembler->stop();        // 停止帧组装器

    // 等待所有线程退出
    for (auto &t : mThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    mThreads.clear();

    std::cout << "[TasksManager] All tasks stopped.\n";
}

void TasksManager::pushDataFrame(const DataFrame& frame)
{
    if (mComTask) {
        mComTask->pushDataFrame(frame);
    }
}

/**
 * @brief 发送任务，替换原先的 ComTask::sendThreadFunc
 */
void TasksManager::sendTaskFunc()
{
    mComTask->sendThreadFunc();
}

/**
 * @brief 接收任务，替换原先的 ComTask::recvThreadFunc
 */
void TasksManager::recvTaskFunc()
{
    mComTask->recvThreadFunc();
}

void TasksManager::sendHeartBeat()
{
    // 每隔3s发送一次心跳包
    while (mIsRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "[TasksManager] Sending heartbeat...\n";
        DataFrame heartbeatFrame = createHeartbeatFrame();
        if (mComTask) {
            mComTask->pushDataFrame(heartbeatFrame);
        }
    }
}

/**
 * @brief 取帧任务，替换原先的 FrameAssembler::run
 */
void TasksManager::assembleCompleteFrame()
{
    mFrameAssembler->run();
}

/**
 * @brief 解析回复帧任务，替换原先的 ReplyFrameDecoder::runDecodeThread
 */
void TasksManager::decodeReplyFrame()
{
    mReplyDecoder->runDecodeThread();
}