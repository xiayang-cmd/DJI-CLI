#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <string>
#include "TelemetryDataBuf-new.pb.h"

class TelemetryUI
{
public:
    TelemetryUI();
    ~TelemetryUI();

    // 启动 UI 线程
    void start();

    // 停止 UI 线程并清理
    void stop();

    // 从外部更新 TelemetryData
    void update(const TelemetryData& data);

    // -------------------- 新增：从外部更新 UavState --------------------
    void updateUavState(const UavState& state);

private:
    // 线程函数：GLFW + ImGui 初始化 -> 主循环 -> 清理
    void uiThreadFunc();

    // 渲染 ImGui 界面
    void render();

private:
    std::atomic_bool m_stop{false};
    std::thread      m_thread;

    // 保护 TelemetryData 的读写
    std::mutex       m_dataMutex;
    TelemetryData    m_data;

    // -------------------- 新增：保护 UavState 的读写 --------------------
    std::mutex       m_uavStateMutex;
    UavState         m_uavState;
};
