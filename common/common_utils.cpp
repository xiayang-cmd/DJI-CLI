#include "common_utils.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 函数：从 JSON 文件中获取服务器地址和端口
ServerConfig getServerConfig(const std::string& filename) {
    std::ifstream input_file(filename);

    if (!input_file.is_open()) {
        throw std::runtime_error("Could not open the file: " + filename);
    }

    json j;
    input_file >> j;  // 读取 JSON 数据

    // 提取服务器地址和端口
    ServerConfig server_cfg;
    try {
        server_cfg.ip   = j.at("server").get<std::string>();    // 获取服务器IP地址
        server_cfg.port = j.at("port").get<int>();              // 获取端口号
        server_cfg.is_valid = true; // 标记配置有效
    } catch (const json::exception& e) {
        throw std::runtime_error("Error reading JSON fields: " + std::string(e.what()));
    }

    return server_cfg;
}

// ------------------ 基础类型转大端模式的工具函数 ------------------
std::vector<uint8_t> floatToBigEndian(float value) {
    std::vector<uint8_t> data(4);
    uint32_t temp;
    std::memcpy(&temp, &value, sizeof(float));
    data[0] = static_cast<uint8_t>((temp >> 24) & 0xFF);
    data[1] = static_cast<uint8_t>((temp >> 16) & 0xFF);
    data[2] = static_cast<uint8_t>((temp >> 8) & 0xFF);
    data[3] = static_cast<uint8_t>(temp & 0xFF);
    return data;
}

std::vector<uint8_t> doubleToBigEndian(double value) {
    std::vector<uint8_t> data(8);
    uint64_t temp;
    std::memcpy(&temp, &value, sizeof(double));
    data[0] = static_cast<uint8_t>((temp >> 56) & 0xFF);
    data[1] = static_cast<uint8_t>((temp >> 48) & 0xFF);
    data[2] = static_cast<uint8_t>((temp >> 40) & 0xFF);
    data[3] = static_cast<uint8_t>((temp >> 32) & 0xFF);
    data[4] = static_cast<uint8_t>((temp >> 24) & 0xFF);
    data[5] = static_cast<uint8_t>((temp >> 16) & 0xFF);
    data[6] = static_cast<uint8_t>((temp >> 8) & 0xFF);
    data[7] = static_cast<uint8_t>(temp & 0xFF);
    return data;
}

std::vector<uint8_t> uint16ToBigEndian(uint16_t value) {
    std::vector<uint8_t> data(2);
    data[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[1] = static_cast<uint8_t>(value & 0xFF);
    return data;
}

std::vector<uint8_t> uint32ToBigEndian(uint32_t value) {
    std::vector<uint8_t> data(4);
    data[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    data[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    data[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[3] = static_cast<uint8_t>(value & 0xFF);
    return data;
}