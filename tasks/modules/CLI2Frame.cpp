#include "CLI2Frame.h"

// ---------- 这里根据实现需要，合理包含头文件 ----------
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdio>        // 如果你使用 printf/puts 等C风格IO，则需要
#include <chrono>
#include <unordered_map>
#include <stdexcept>     // 若需要使用异常 (std::stoi, std::stod 等可能抛出)
#include "common_utils.h"
#include "GimbalJoystickController.h"
#include <google/protobuf/util/json_util.h>

// ------------------ 打印字节帧数据 ------------------
void print_hex(const DataFrame& data) {
    std::cout << "Frame size: " << data.size() << ", hex data: ";
    for (auto b : data) {
        printf("%02X ", b);
    }
    printf("\n");
}

// ------------------ 心跳帧单独处理(字段不一样) ------------------
DataFrame createHeartbeatFrame() {
    DataFrame frame {
        0x74, 0x79,   // 帧头
        0x00, 0x09,   // 数据长度
        0x02,         // 指令编号(心跳帧约定为0x02)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // 时间戳 8字节
    };

    // 获取当前时间戳（秒级）
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

    // 填充时间戳
    for (int i = 0; i < 8; i++) {
        frame[5 + i] = (currentTimestamp >> ((7 - i) * 8)) & 0xFF;
    }

    return frame;
}

// ------------------ 注册帧单独处理(字段不一样) ------------------
DataFrame createRegisterFrame(uint32_t companyId, const std::string& accessToken)
{
    // 帧头
    DataFrame frame { 0x74, 0x79 };

    // 先暂时插入2字节的 data_length (占位)
    frame.push_back(0x00);
    frame.push_back(0x00);

    // 注册帧不带 SN

    // 指令编号(注册帧约定为0x01)
    frame.push_back(0x01);

    // 注册帧不带 加密标志

    // 填充 鉴权相关信息: [companyId(4B)] + [accessToken(NB)]
    std::vector<uint8_t> companyIdData = uint32ToBigEndian(companyId);
    frame.insert(frame.end(), companyIdData.begin(), companyIdData.end());
    frame.insert(frame.end(), accessToken.begin(), accessToken.end());

    // 计算并回填 data_length
    uint16_t length = frame.size() - 4;
    frame[2] = static_cast<uint8_t>((length >> 8) & 0xFF);
    frame[3] = static_cast<uint8_t>(length & 0xFF);

    return frame;
}

// ------------------ 统一构建控制帧(帧头: 0x74 0x79) ------------------
// 说明：以下是“控制帧”结构: [帧头2B][数据长度2B][SN号15B][指令编号1B][加密标志1B][动作编号1B][动作参数(NB)]
//
// 其中，SN号在此示例直接写死，也可以从配置中加载。指令编号默认为 0xD1(仅示例)。
DataFrame createControlFrame(uint8_t actionId, const std::vector<uint8_t>& actionParam)
{
    // 1. 定义协议中的默认值或常量
    constexpr uint8_t FRAME_HEADER[2] = { 0x74, 0x79 };  // 帧头
    constexpr uint8_t DEFAULT_COMMAND_ID      = 0xD1;    // 指令编号
    constexpr uint8_t DEFAULT_ENCRYPTION_FLAG = 0x00;    // 加密标志(0x00=不加密)

    // SN号(15B)，这里示例直接硬编码
    static const std::vector<uint8_t> SN_NUMBER = {
        'D', 'B', 'M', '2', '5', '0', '9', '7', '4', '0', '6', '5', '0', '0', '8'
    };

    // 2. 开始组装数据帧
    DataFrame frame;

    // (1) 插入帧头
    frame.insert(frame.end(), std::begin(FRAME_HEADER), std::end(FRAME_HEADER));

    // (2) 为“数据长度”字段预留2个字节，待后面计算回填
    frame.push_back(0x00);
    frame.push_back(0x00);

    // (3) 插入 SN 号(15字节)
    frame.insert(frame.end(), SN_NUMBER.begin(), SN_NUMBER.end());

    // (4) 插入指令编号
    frame.push_back(DEFAULT_COMMAND_ID);

    // (5) 插入加密标志
    frame.push_back(DEFAULT_ENCRYPTION_FLAG);

    // (6) 插入动作编号
    frame.push_back(actionId);

    // (7) 插入动作参数（原 actionParam）
    frame.insert(frame.end(), actionParam.begin(), actionParam.end());

    // 3. 计算并回填“数据长度”（不包含帧头2字节 + 数据长度本身2字节）
    //    也就是从SN号开始到最后的所有字段大小
    uint16_t length = static_cast<uint16_t>(frame.size() - 4);
    frame[2] = static_cast<uint8_t>((length >> 8) & 0xFF);
    frame[3] = static_cast<uint8_t>((length & 0xFF));

    return frame;
}




// ------------------ 航线规划的示例(仅占位) ------------------
static std::vector<uint8_t> mockRoutePlanData()
{
    // 在此拼装航线数据
    RouteDataModule routeModule;
    PlanLineData planData;

    // 从 JSON 文件加载 PlanLineData
    if (!routeModule.jsonFileToPlanLineData("../config/planData.json", planData)) {
        std::cerr << "Error: Failed to load planData from JSON.\n";
        return {};
    }
    // // 打印 planData 的json格式
    // // 使用 Protobuf 的 JsonPrintOptions 配置输出选项
    // google::protobuf::util::JsonPrintOptions options;
    // options.add_whitespace = true;              // 添加空格、换行等美化输出
    // options.always_print_primitive_fields = true;
    // options.preserve_proto_field_names = true;

    // std::string jsonStr;
    // auto status = google::protobuf::util::MessageToJsonString(planData, &jsonStr, options);
    // if (!status.ok()) {
    //     std::cerr << "Failed to convert PlanLineData to JSON string: " 
    //               << status.ToString() << std::endl;
    //     return {};
    // }
    // std::cout << jsonStr << std::endl;

    // 将 PlanLineData 序列化为字节数组
    std::string serialized_data;
    if (!planData.SerializeToString(&serialized_data)) {
        std::cerr << "Error: Failed to serialize PlanLineData.\n";
        return {};
    }
    // 转换为 uint8_t vector
    std::vector<uint8_t> route_data(serialized_data.begin(), serialized_data.end());

    return route_data;
}

// ------------------ 解析用户输入，生成 DataFrame ------------------
DataFrame parseCommand(const std::string& line)
{
    // 将命令行拆分成 tokens
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    // 空行或只有空格
    if (tokens.empty()) {
        return {};
    }

    // 特殊命令: heartbeat
    if (tokens[0] == "heartbeat") {
        return createHeartbeatFrame();
    }

    // 特殊命令: register
    if (tokens[0] == "register") {
        // 语法: register <companyId> <accessToken(字符串)>
        if (tokens.size() < 3) {
            std::cerr << "Usage: register <companyId> <accessToken>\n";
            return {};
        }
        uint32_t companyId = std::stoul(tokens[1]);
        std::string accessToken = tokens[2];
        return createRegisterFrame(companyId, accessToken);
    }

    // ---------- 以下是常规的控制帧: createControlFrame(actionId, actionParam) ----------

    // 手动飞行 / 无人机操控相关
    if (tokens[0] == "takeoff") {
        // 语法: takeoff <height>
        if (tokens.size() < 2) {
            std::cerr << "Usage: takeoff <height>\n";
            return {};
        }
        float height = std::stof(tokens[1]);
        auto actionParam = floatToBigEndian(height);
        return createControlFrame(0x11, actionParam);
    }
    else if (tokens[0] == "land") {
        // land / land cancel / land force
        if (tokens.size() == 1) {
            // land
            return createControlFrame(0x14, {});
        } else if (tokens[1] == "cancel") {
            return createControlFrame(0x15, {});
        } else if (tokens[1] == "force") {
            return createControlFrame(0x29, {});
        }
    }
    else if (tokens[0] == "rth") {
        // rth / rth cancel
        if (tokens.size() == 1) {
            // rth
            return createControlFrame(0x12, {});
        } else if (tokens[1] == "cancel") {
            return createControlFrame(0x13, {});
        }
    }
    else if (tokens[0] == "control") {
        // 语法: control <fb_speed> <lr_speed> <ud_speed> <yaw_angle> <time_ms>
        // 或 control authority <0|1> 
        if (tokens.size() >= 2 && tokens[1] == "authority") {
            // control authority <0|1>
            if (tokens.size() < 3) {
                std::cerr << "Usage: control authority <0|1>\n";
                return {};
            }
            uint8_t control_type = static_cast<uint8_t>(std::stoi(tokens[2]));
            return createControlFrame(0x30, {control_type});
        }
        if (tokens.size() < 6) {
            std::cerr << "Usage: control <fb_speed> <lr_speed> <ud_speed> <yaw_angle> <time_ms>\n";
            return {};
        }
        float fb = std::stof(tokens[1]);
        float lr = std::stof(tokens[2]);
        float ud = std::stof(tokens[3]);
        float yaw = std::stof(tokens[4]);
        uint16_t t  = static_cast<uint16_t>(std::stoi(tokens[5]));
        
        std::vector<uint8_t> data;
        auto fb_data   = floatToBigEndian(fb);
        auto lr_data   = floatToBigEndian(lr);
        auto ud_data   = floatToBigEndian(ud);
        auto yaw_data  = floatToBigEndian(yaw);
        auto time_data = uint16ToBigEndian(t);

        data.insert(data.end(), fb_data.begin(), fb_data.end());
        data.insert(data.end(), lr_data.begin(), lr_data.end());
        data.insert(data.end(), ud_data.begin(), ud_data.end());
        data.insert(data.end(), yaw_data.begin(), yaw_data.end());
        data.insert(data.end(), time_data.begin(), time_data.end());

        return createControlFrame(0x28, data);
    }
    else if (tokens[0] == "goto") {
        // goto <longitude> <latitude> <alt> <speed> <mode> 
        // 或 goto stop
        if (tokens.size() == 1) {
            std::cerr << "Usage: goto <lon> <lat> <alt> <speed> <mode> or goto stop\n";
            return {};
        }
        if (tokens[1] == "stop") {
            return createControlFrame(0x3A, {});
        }
        if (tokens.size() < 6) {
            std::cerr << "Usage: goto <lon> <lat> <alt> <speed> <mode>\n";
            return {};
        }
        double lon = std::stod(tokens[1]);
        double lat = std::stod(tokens[2]);
        float alt  = std::stof(tokens[3]);
        float spd  = std::stof(tokens[4]);
        uint8_t mode = static_cast<uint8_t>(std::stoi(tokens[5]));

        std::vector<uint8_t> data;
        auto lon_data = doubleToBigEndian(lon);
        auto lat_data = doubleToBigEndian(lat);
        auto alt_data = floatToBigEndian(alt);
        auto spd_data = floatToBigEndian(spd);

        data.insert(data.end(), lon_data.begin(), lon_data.end());
        data.insert(data.end(), lat_data.begin(), lat_data.end());
        data.insert(data.end(), alt_data.begin(), alt_data.end());
        data.insert(data.end(), spd_data.begin(), spd_data.end());
        data.push_back(mode);

        return createControlFrame(0x39, data);
    }
    else if (tokens[0] == "brake") {
        // brake <0|1>
        if (tokens.size() < 2) {
            std::cerr << "Usage: brake <0|1>\n";
            return {};
        }
        uint8_t type = static_cast<uint8_t>(std::stoi(tokens[1]));
        return createControlFrame(0x32, {type});
    }

    // 航线飞行
    else if (tokens[0] == "route") {
        // plan / start / pause / resume / stop
        if (tokens.size() < 2) {
            std::cerr << "Usage: route <plan|start|pause|resume|stop>\n";
            return {};
        }
        if (tokens[1] == "plan") {
            auto route_data = mockRoutePlanData(); 
            return createControlFrame(0x10, route_data);
        } else if (tokens[1] == "start") {
            return createControlFrame(0x17, {});
        } else if (tokens[1] == "pause") {
            return createControlFrame(0x18, {});
        } else if (tokens[1] == "resume") {
            return createControlFrame(0x19, {});
        } else if (tokens[1] == "stop") {
            return createControlFrame(0x20, {});
        }
    }

    // 相机控制
    else if (tokens[0] == "camera") {
        if (tokens.size() < 2) {
            std::cerr << "Usage: camera <shot|video|zoom|focus|laser|measure|switch|source|mode|format|photortp> ...\n";
            return {};
        }
        if (tokens[1] == "shot") {
            // camera shot / camera shot auto start / camera shot auto stop
            if (tokens.size() == 2) {
                // 拍照
                return createControlFrame(0x23, {});
            } else if (tokens.size() >= 3 && tokens[2] == "auto") {
                if (tokens.size() >= 4 && tokens[3] == "start") {
                    // 开始自动拍照
                    if (tokens.size() >= 5) {
                        // 如果有参数，假设是拍照间隔
                        uint8_t interval = static_cast<uint8_t>(std::stoi(tokens[4]));
                        return createControlFrame(0x38, {interval});
                    }
                    return createControlFrame(0x38, {3}); // 默认间隔3秒
                } else if (tokens.size() >= 4 && tokens[3] == "stop") {
                    // 停止自动拍照
                    return createControlFrame(0x38, {0});
                }
            }
        }
        else if (tokens[1] == "video") {
            // camera video start / camera video stop
            if (tokens.size() < 3) {
                std::cerr << "Usage: camera video <start|stop>\n";
                return {};
            }
            if (tokens[2] == "start") {
                return createControlFrame(0x24, {});
            } else if (tokens[2] == "stop") {
                return createControlFrame(0x25, {});
            }
        }
        else if (tokens[1] == "zoom") {
            // camera zoom in|out|reset|stop
            // or camera zoom <level>
            if (tokens.size() < 3) {
                std::cerr << "Usage: camera zoom <in|out|reset|stop> or camera zoom <level>\n";
                return {};
            }
            if (tokens[2] == "in") {
                return createControlFrame(0x0A, {});
            } else if (tokens[2] == "out") {
                return createControlFrame(0x0B, {});
            } else if (tokens[2] == "reset") {
                return createControlFrame(0x0F, {});
            } else if (tokens[2] == "stop") {
                return createControlFrame(0xFF, {});
            } else {
                // 当作数值
                uint8_t zoom_level = static_cast<uint8_t>(std::stoi(tokens[2]));
                return createControlFrame(0x0D, {zoom_level});
            }
        }
        else if (tokens[1] == "focus") {
            // camera focus <x> <y>
            if (tokens.size() < 4) {
                std::cerr << "Usage: camera focus <x> <y>\n";
                return {};
            }
            float x = std::stof(tokens[2]);
            float y = std::stof(tokens[3]);
            auto x_data = floatToBigEndian(x);
            auto y_data = floatToBigEndian(y);
            std::vector<uint8_t> data;
            data.insert(data.end(), x_data.begin(), x_data.end());
            data.insert(data.end(), y_data.begin(), y_data.end());
            return createControlFrame(0x1B, data);
        }
        else if (tokens[1] == "laser") {
            // camera laser <0|1>
            if (tokens.size() < 3) {
                std::cerr << "Usage: camera laser <0|1>\n";
                return {};
            }
            uint8_t laser = static_cast<uint8_t>(std::stoi(tokens[2]));
            return createControlFrame(0x1A, {laser});
        }
        else if (tokens[1] == "measure") {
            // camera measure <x> <y>
            if (tokens.size() < 4) {
                std::cerr << "Usage: camera measure <x> <y>\n";
                return {};
            }
            float x = std::stof(tokens[2]);
            float y = std::stof(tokens[3]);
            auto x_data = floatToBigEndian(x);
            auto y_data = floatToBigEndian(y);
            std::vector<uint8_t> data;
            data.insert(data.end(), x_data.begin(), x_data.end());
            data.insert(data.end(), y_data.begin(), y_data.end());
            // 注意这个action_id和focus一样, 在你的实际协议里可能不同
            return createControlFrame(0xFB, data);
        }
        else if (tokens[1] == "switch") {
            // camera switch <0|1>
            if (tokens.size() < 3) {
                std::cerr << "Usage: camera switch <0|1>\n";
                return {};
            }
            uint8_t camType = static_cast<uint8_t>(std::stoi(tokens[2]));
            return createControlFrame(0x27, {camType});
        }
        else if (tokens[1] == "source") {
            // camera source <0|1|2>
            if (tokens.size() < 3) {
                std::cerr << "Usage: camera source <0|1|2>\n";
                return {};
            }
            uint8_t src = static_cast<uint8_t>(std::stoi(tokens[2]));
            return createControlFrame(0x26, {src});
        }
        else if (tokens[1] == "mode") {
            // camera mode <1|2>
            if (tokens.size() < 3) {
                std::cerr << "Usage: camera mode <1|2>\n";
                return {};
            }
            uint8_t mode = static_cast<uint8_t>(std::stoi(tokens[2]));
            return createControlFrame(0x22, {mode});
        }
        else if (tokens[1] == "format") {
            // camera format
            return createControlFrame(0x40, {});
        }
        else if (tokens[1] == "photortp") {
            // camera photortp <0|1>
            if (tokens.size() < 3) {
                std::cerr << "Usage: camera photortp <0|1>\n";
                return {};
            }
            uint8_t on_off = static_cast<uint8_t>(std::stoi(tokens[2]));
            return createControlFrame(0x43, {on_off});
        }
    }

    // 云台控制
    else if (tokens[0] == "gimbal") {
        // gimbal move abs <pitch> <roll> <yaw>
        // gimbal move speed <pitch_speed> <roll_speed> <yaw_speed> <time_ms>
        // gimbal follow <mode>
        // gimbal set <0|1|2|3>
        if (tokens.size() < 2) {
            std::cerr << "Usage: gimbal <move|follow|set> ...\n";
            return {};
        }
        if (tokens[1] == "move") {
            if (tokens.size() < 3) {
                std::cerr << "Usage: gimbal move <joystick|abs|speed> ...\n";
                return {};
            }
            if (tokens[2] == "joystick"){
                // 进入“云台摇杆模式”
                GimbalJoystickController controller;
                controller.run();

                // run() 结束后, 就回到 CLI 普通模式
                return {};
            }
            if (tokens[2] == "abs") {
                // gimbal move abs <pitch> <roll> <yaw>
                if (tokens.size() < 6) {
                    std::cerr << "Usage: gimbal move abs <pitch> <roll> <yaw>\n";
                    return {};
                }
                float pitch = std::stof(tokens[3]);
                float roll  = std::stof(tokens[4]);
                float yaw   = std::stof(tokens[5]);

                std::vector<uint8_t> data;
                auto pitch_data = floatToBigEndian(pitch);
                auto roll_data  = floatToBigEndian(roll);
                auto yaw_data   = floatToBigEndian(yaw);
                data.insert(data.end(), pitch_data.begin(), pitch_data.end());
                data.insert(data.end(), roll_data.begin(), roll_data.end());
                data.insert(data.end(), yaw_data.begin(), yaw_data.end());

                // 示例动作编号: 0x09
                return createControlFrame(0x09, data);

            } else if (tokens[2] == "speed") {
                // gimbal move speed <pitch_speed> <roll_speed> <yaw_speed> <time_ms>
                if (tokens.size() < 7) {
                    std::cerr << "Usage: gimbal move speed <pitch_spd> <roll_spd> <yaw_spd> <time_ms>\n";
                    return {};
                }
                float pitch_spd = std::stof(tokens[3]);
                float roll_spd  = std::stof(tokens[4]);
                float yaw_spd   = std::stof(tokens[5]);
                uint16_t t      = static_cast<uint16_t>(std::stoi(tokens[6]));

                std::vector<uint8_t> data;
                auto pitch_data = floatToBigEndian(pitch_spd);
                auto roll_data  = floatToBigEndian(roll_spd);
                auto yaw_data   = floatToBigEndian(yaw_spd);
                auto time_data  = uint16ToBigEndian(t);

                data.insert(data.end(), pitch_data.begin(), pitch_data.end());
                data.insert(data.end(), roll_data.begin(), roll_data.end());
                data.insert(data.end(), yaw_data.begin(), yaw_data.end());
                data.insert(data.end(), time_data.begin(), time_data.end());

                // 示例动作编号: 0xF4
                return createControlFrame(0xF4, data);
            }
        }
        else if (tokens[1] == "follow") {
            // gimbal follow <mode>
            if (tokens.size() < 3) {
                std::cerr << "Usage: gimbal follow <1|2|3>\n";
                return {};
            }
            uint8_t mode = static_cast<uint8_t>(std::stoi(tokens[2]));
            return createControlFrame(0x1C, {mode});
        }
        else if (tokens[1] == "set") {
            // gimbal set <0|1|2|3>
            if (tokens.size() < 3) {
                std::cerr << "Usage: gimbal set <0|1|2|3>\n";
                return {};
            }
            uint8_t attitude = static_cast<uint8_t>(std::stoi(tokens[2]));
            return createControlFrame(0x1D, {attitude});
        }
    }

    // 无人机设置
    else if (tokens[0] == "obstacle") {
        // obstacle horizontal <0|1>
        // obstacle up <0|1>
        // obstacle down <0|1>
        if (tokens.size() < 3) {
            std::cerr << "Usage: obstacle <horizontal|up|down> <0|1>\n";
            return {};
        }
        uint8_t on_off = static_cast<uint8_t>(std::stoi(tokens[2]));
        if (tokens[1] == "horizontal") {
            return createControlFrame(0x35, {on_off});
        } else if (tokens[1] == "up") {
            return createControlFrame(0x36, {on_off});
        } else if (tokens[1] == "down") {
            return createControlFrame(0x37, {on_off});
        }
    }
    else if (tokens[0] == "home") {
        // home set <longitude> <latitude>
        // home height <uint16>
        if (tokens.size() < 2) {
            std::cerr << "Usage: home <set|height> ...\n";
            return {};
        }
        if (tokens[1] == "set") {
            if (tokens.size() < 4) {
                std::cerr << "Usage: home set <lon> <lat>\n";
                return {};
            }
            double lon = std::stod(tokens[2]);
            double lat = std::stod(tokens[3]);
            std::vector<uint8_t> data;
            auto lon_data = doubleToBigEndian(lon);
            auto lat_data = doubleToBigEndian(lat);
            data.insert(data.end(), lon_data.begin(), lon_data.end());
            data.insert(data.end(), lat_data.begin(), lat_data.end());
            return createControlFrame(0x31, data);
        } else if (tokens[1] == "height") {
            if (tokens.size() < 3) {
                std::cerr << "Usage: home height <uint16>\n";
                return {};
            }
            uint16_t h = static_cast<uint16_t>(std::stoi(tokens[2]));
            auto h_data = uint16ToBigEndian(h);
            return createControlFrame(0x21, h_data);
        }
    }

    // 未知命令
    std::cerr << "Unknown command: " << tokens[0] << "\n";
    return {};
}

// // ------------------ main函数演示CLI循环 ------------------
// int main()
// {
//     std::cout << "CLI 地面站示例. 输入命令, 如: takeoff 10\n";
//     std::cout << "输入 exit 退出.\n";

//     while (true) {
//         std::cout << "pxh> ";
//         std::string line;
//         if (!std::getline(std::cin, line)) {
//             break; // 处理 Ctrl+D 等情况
//         }
//         if (line == "exit") {
//             break;
//         }
//         // 解析并构建数据帧
//         DataFrame frame = parseCommand(line);
//         if (!frame.empty()) {
//             print_hex(frame);
//         }
//     }
//     return 0;
// }
