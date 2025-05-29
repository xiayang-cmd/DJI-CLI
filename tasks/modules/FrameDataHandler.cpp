#include "FrameDataHandler.h"

FrameDataHandler::FrameDataHandler()
{
    // 构造函数，如有必要，可在此进行成员变量初始化
    // 初始化 TelemetryUI
    m_telemetryUI.start(); // 启动 UI 线程
    std::cout << "UI thread started." << std::endl;

}

FrameDataHandler::~FrameDataHandler()
{
    // 析构函数，如有必要，可在此进行资源释放
    m_telemetryUI.stop(); // 停止 UI 线程
}

void FrameDataHandler::handleFrameData(uint8_t cmdId, const uint8_t* data, uint16_t length)
{
    switch (cmdId)
    {
    case 0xD1:
        handleD1(data, length);
        break;
    case 0xA9:
        handleA9(data, length);
        break;
    case 0xA8:
        handleA8(data, length);
        break;
    default:
        std::cout << "[FrameDataHandler] Unknown cmdId = 0x"
                  << std::hex << static_cast<int>(cmdId)
                  << ", length = " << std::dec << length << std::endl;
        break;
    }
}

void FrameDataHandler::handleD1(const uint8_t* data, uint16_t length)
{
    // // TODO: 处理 0xD1 类型数据的实际业务逻辑(回复数据)
    // std::cout << "[FrameDataHandler] Handling 0xD1 data, length = "
    //           << length << std::endl;

    if (length < 1 + 1 + 1 + 4 + 15)
    {
        std::cerr << "[FrameDataHandler] handleD1 error: data length too short."
                << std::endl;
        return;
    }

    // 加密标志
    uint8_t encryptionFlag = data[0];
    // 动作编号
    uint8_t actionNumber   = data[1];
    // 执行结果
    uint8_t execResult     = data[2];

    // 错误码 (此处假设协议中是大端存储)
    uint32_t errorCode =
        (static_cast<uint32_t>(data[3]) << 24) |
        (static_cast<uint32_t>(data[4]) << 16) |
        (static_cast<uint32_t>(data[5]) <<  8) |
        (static_cast<uint32_t>(data[6]));

    // 云盒 SN，长度 15 字节，这里用字符串来存储和打印
    std::string cloudBoxSN(reinterpret_cast<const char*>(&data[7]), 15);

    // 打印解析得到的信息
    std::cout << "[FrameDataHandler][0xD1] "
    << "CloudBoxSN: " << cloudBoxSN
    << ", EncryptionFlag: " << static_cast<int>(encryptionFlag)
    << ", ActionNumber: "   << static_cast<int>(actionNumber)
    << ", ExecResult: "     << static_cast<int>(execResult)
    << ", ErrorCode: "      << errorCode
    << std::endl;

}

void FrameDataHandler::handleA9(const uint8_t* data, uint16_t length)
{
    // // TODO: 处理 0xA9 类型数据的实际业务逻辑(遥测数据)
    // std::cout << "[FrameDataHandler] Handling 0xA9 data, length = "
    //           << length << std::endl;
    // 将data转化为string，再通过protobuf反序列化
    TelemetryData telemetryData;
    std::string dataStr(reinterpret_cast<const char*>(data), length);
    if (telemetryData.ParseFromString(dataStr)) {
        // std::cout << "Parsed TelemetryDataBuf successfully." << std::endl;
        // 这里可以进一步处理 telemetryData
        m_telemetryUI.update(telemetryData); // 更新 UI 显示
    } else {
        std::cerr << "Failed to parse TelemetryDataBuf." << std::endl;
    }
}

void FrameDataHandler::handleA8(const uint8_t* data, uint16_t length)
{
    // // TODO: 处理 0xA8 类型数据的实际业务逻辑(无人机状态数据)
    // std::cout << "[FrameDataHandler] Handling 0xA8 data, length = "
    //           << length << std::endl;
    // 将data转化为string，再通过protobuf反序列化
    UavState uavState;
    std::string dataStr(reinterpret_cast<const char*>(data), length);
    if (uavState.ParseFromString(dataStr)) {
        // std::cout << "Parsed UavState successfully." << std::endl;
        // 这里可以进一步处理 uavState
        m_telemetryUI.updateUavState(uavState); // 更新 UI 显示
    } else {
        std::cerr << "Failed to parse UavState." << std::endl;
    }
}
