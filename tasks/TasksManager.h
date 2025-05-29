#pragma once

#include "CLI2Frame.h"
#include "FrameAssembler.h"
#include "ReplyFrameDecoder.h"
#include "FrameDataHandler.h"
#include "com_task.h"
#include "common_types.h"
#include "common_utils.h"
#include "utils/CLinuxTCPCom.h"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

/**
 * @brief 统一管理各个任务的启动、停止等
 */
class TasksManager {
public:
  TasksManager();
  ~TasksManager();

  /**
   * @brief 初始化所有任务所需资源
   */
  bool initAllTasks();

  /**
   * @brief 启动所有任务（线程）
   */
  void startAllTasks();

  /**
   * @brief 停止所有任务（线程）
   */
  void stopAllTasks();

  /**
   * @brief 往发送队列里塞入一帧数据
   */
  void pushDataFrame(const DataFrame &frame);

private:
  /**
   * @brief 发送数据线程函数（原 ComTask::sendThreadFunc）
   */
  void sendTaskFunc();

  /**
   * @brief 接收数据线程函数（原 ComTask::recvThreadFunc）
   */
  void recvTaskFunc();

  /**
   * @brief 从原始数据块构建完整数据帧的函数
   */
  void assembleCompleteFrame();

  /**
   * @brief 解析回复帧的线程函数（原 ReplyFrameDecoder::runDecodeThread）
   */
  void decodeReplyFrame();

  /**
   * @brief 发送心跳包
   */
  void sendHeartBeat();

private:
  std::atomic<bool>         mIsRunning;   ///< 表示当前任务是否处于运行状态
  std::vector<std::thread>  mThreads;     ///< 线程容器

  std::unique_ptr<CLinuxTCPCom>       mTcpCom;          ///< TCP通信类封装

  std::unique_ptr<ComTask>            mComTask;         ///< 通信任务
  std::unique_ptr<FrameAssembler>     mFrameAssembler;  ///< 帧组装器
  std::unique_ptr<ReplyFrameDecoder>  mReplyDecoder;    ///< 帧解析器
};
