#ifndef CLI2FRAME_H_
#define CLI2FRAME_H_

#include <vector>
#include <cstdint>
#include <string>
#include "routeDataModule.h"

// 在头文件中仅包含与声明直接相关的头；与实现无关的头放到 .cpp 即可
// 定义我们的 DataFrame 类型别名
using DataFrame = std::vector<uint8_t>;

constexpr uint32_t DEFAULT_COMPANY_ID = 209938; // 默认公司ID
constexpr char DEFAULT_ACCESS_TOKEN[] = "4c08aeb6e96dcefbd2d705faab1a3c00afe20ab3f050e06e01d655ecef7d13be95225bba5b92187127a20bba5b7454fdc5f303eb60d756ec046958e16284558f";

/**
 * @brief 打印字节帧数据
 */
void print_hex(const DataFrame& data);

/**
 * @brief 生成“心跳帧”
 */
DataFrame createHeartbeatFrame();

/**
 * @brief 生成“注册帧”
 * @param companyId    公司ID
 * @param accessToken  访问令牌
 */
DataFrame createRegisterFrame(uint32_t companyId = DEFAULT_COMPANY_ID, const std::string& accessToken = DEFAULT_ACCESS_TOKEN);

/**
 * @brief 生成“控制帧”
 * @param actionId   动作编号
 * @param actionParam 动作参数
 */
DataFrame createControlFrame(uint8_t actionId, const std::vector<uint8_t>& actionParam);

/**
 * @brief 根据用户输入字符串，解析并生成对应的 DataFrame
 * @param line 用户输入的命令行字符串
 * @return 构建好的数据帧；若解析失败或无效，返回空的 DataFrame
 */
DataFrame parseCommand(const std::string& line);

#endif // CLI2FRAME_H_
