#include "routeDataModule.h"

#include <fstream>
#include <iostream>

#include <google/protobuf/util/json_util.h> // MessageToJsonString, JsonStringToMessage
#include <nlohmann/json.hpp>               // 仅用于再次 parse/dump 以实现美化输出

bool RouteDataModule::jsonFileToPlanLineData(const std::string& jsonFilePath,
                                             PlanLineData& planData)
{
    namespace pbutil = google::protobuf::util;

    // 1. 读取 JSON 文件内容到字符串
    std::ifstream ifs(jsonFilePath);
    if (!ifs.is_open()) {
        std::cerr << "[RouteDataModule] Failed to open file: " << jsonFilePath << std::endl;
        return false;
    }
    std::string jsonStr((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
    ifs.close();

    // 2. 使用 protobuf 的 JsonStringToMessage 将 JSON 字符串转为 Protobuf 对象
    pbutil::JsonParseOptions parseOptions;
    parseOptions.ignore_unknown_fields = false; // 如果文件里有未知字段，你可选择是否忽略
    auto parseStatus = pbutil::JsonStringToMessage(jsonStr, &planData, parseOptions);
    if (!parseStatus.ok()) {
        std::cerr << "[RouteDataModule] Failed to parse JSON to PlanLineData: "
                  << parseStatus.ToString() << std::endl;
        return false;
    }

    std::cout << "[RouteDataModule] jsonFileToPlanLineData: [" << jsonFilePath << "] done.\n";
    return true;
}

bool RouteDataModule::planLineDataToJsonFile(const PlanLineData& planData,
                                             const std::string& jsonFilePath)
{
    namespace pbutil = google::protobuf::util;

    // 1. 将 Protobuf 对象转换为 JSON 字符串（带格式选项）
    std::string protoJsonStr;
    pbutil::JsonPrintOptions jpOptions;
    jpOptions.add_whitespace = true;            // 美化输出
    jpOptions.always_print_primitive_fields = true;
    jpOptions.preserve_proto_field_names = true;

    auto status = pbutil::MessageToJsonString(planData, &protoJsonStr, jpOptions);
    if (!status.ok()) {
        std::cerr << "[RouteDataModule] Failed to convert PlanLineData to JSON: "
                  << status.ToString() << std::endl;
        return false;
    }

    // 2. 使用 nlohmann::json 再解析一次，仅用于更优雅的 dump(4) 格式化输出
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(protoJsonStr);
    } catch (const std::exception& e) {
        std::cerr << "[RouteDataModule] Failed to parse protoJsonStr with nlohmann: "
                  << e.what() << std::endl;
        return false;
    }

    // 3. 写入 JSON 文件
    std::ofstream ofs(jsonFilePath);
    if (!ofs.is_open()) {
        std::cerr << "[RouteDataModule] Failed to open file for writing: "
                  << jsonFilePath << std::endl;
        return false;
    }
    ofs << j.dump(4); // 4 个空格缩进
    ofs.close();

    std::cout << "[RouteDataModule] planLineDataToJsonFile: [" << jsonFilePath << "] done.\n";
    return true;
}
