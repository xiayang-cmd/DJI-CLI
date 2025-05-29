#ifndef FRAMEDATAHANDLER_H
#define FRAMEDATAHANDLER_H

#include <cstdint>
#include <iostream>
#include "TelemetryDataBuf-new.pb.h"
#include "TelemetryUI.h"

/**
 * @brief 处理不同命令ID对应的帧数据
 */
class FrameDataHandler
{
public:
    FrameDataHandler();
    ~FrameDataHandler();

    /**
     * @brief 根据cmdId处理帧数据
     * @param cmdId  命令ID
     * @param data   源数据指针
     * @param length 源数据长度
     */
    void handleFrameData(uint8_t cmdId, const uint8_t* data, uint16_t length);

private:
    // 以下是针对不同命令ID的处理函数，可以根据业务逻辑做更详细的拆分
    void handleD1(const uint8_t* data, uint16_t length);
    void handleA9(const uint8_t* data, uint16_t length);
    void handleA8(const uint8_t* data, uint16_t length);

private:
    TelemetryUI m_telemetryUI; // 用于显示遥测数据的UI
};

#endif // FRAMEDATAHANDLER_H
