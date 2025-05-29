#pragma once

#include "common_types.h"

/**
 * @brief 从指定的 JSON 配置文件中获取服务器地址和端口
 * @param filename 配置文件名
 * @return 返回包含服务器地址和端口的 ServerConfig 结构体
 */
ServerConfig getServerConfig(const std::string& filename);


std::vector<uint8_t> floatToBigEndian(float value);
std::vector<uint8_t> doubleToBigEndian(double value);
std::vector<uint8_t> uint16ToBigEndian(uint16_t value);
std::vector<uint8_t> uint32ToBigEndian(uint32_t value);

