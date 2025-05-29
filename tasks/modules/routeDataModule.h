#pragma once

#include <string>
#include "TelemetryDataBuf-new.pb.h"

/**
 * @brief 负责将航迹文件(JSON) 转化为基于 protobuf 的 PlanLineData 数据结构（以及反向）。
 */
class RouteDataModule
{
public:
    RouteDataModule() = default;
    ~RouteDataModule() = default;

    /**
     * @brief 从 JSON 文件读取数据并转换为 Protobuf::PlanLineData 对象
     * @param jsonFilePath [in] JSON 文件路径
     * @param planData     [out] 输出的 Protobuf::PlanLineData
     * @return true 表示转换成功；false 表示失败
     */
    bool jsonFileToPlanLineData(const std::string& jsonFilePath,
                                PlanLineData& planData);

    /**
     * @brief 将 PlanLineData 数据写入 JSON 文件
     * @param planData     [in]  要写入的 Protobuf::PlanLineData
     * @param jsonFilePath [in]  目标 JSON 文件路径
     * @return true 表示写入成功；false 表示失败
     */
    bool planLineDataToJsonFile(const PlanLineData& planData,
                                const std::string& jsonFilePath);
};
