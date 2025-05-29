// GimbalJoystickController.cpp

#include "GimbalJoystickController.h"
#include <iostream>
#include <cstdio>
#include <termios.h>
#include <unistd.h>
#include <thread>
#include <atomic>

// (在别的文件里移植的，防止引用混乱)
static DataFrame createControlFrame(uint8_t actionId, const std::vector<uint8_t>& actionParam)
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

// 动作编号(示例值),比如 0x00=回中, 0x01=上, 0x05=下, 0x07=左, 0x03=右
static constexpr uint8_t ACTION_GIMBAL_CENTER = 0x00;
static constexpr uint8_t ACTION_GIMBAL_UP     = 0x01;
static constexpr uint8_t ACTION_GIMBAL_DOWN   = 0x05;
static constexpr uint8_t ACTION_GIMBAL_LEFT   = 0x07;
static constexpr uint8_t ACTION_GIMBAL_RIGHT  = 0x03;


// 返回值表示这帧是否真正入队（true=已入队，false=被丢弃）
bool enqueueWithThrottle(const DataFrame& frame)
{
    using clock = std::chrono::steady_clock;
    static std::atomic<clock::time_point> lastPush{
        clock::now() - std::chrono::milliseconds(200)};   // 保证第一帧能通过

    auto now   = clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPush.load());

    // 间隔不足 200 ms —— 直接丢弃
    if (delta < std::chrono::milliseconds(200))
        return false;

    {
        std::lock_guard<std::mutex> lk(g_queueMutex);
        g_dataFrameQueue.push(frame);
    }
    g_queueCond.notify_one();

    lastPush.store(clock::now());
    return true;
}

void GimbalJoystickController::run()
{
    // 首先将终端切换到原始模式，以便直接捕获按键(含箭头)
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    // 关闭行缓冲(ICANON)和回显(ECHO)
    newt.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO));
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cout << "=== Gimbal Joystick Mode ===\n"
              << "Use arrow keys to move.\n"
              << "Press '0' to recenter.\n"
              << "Press 'q' to quit.\n";

    while (true)
    {
        // getchar() 在原始模式下会立即返回按键值
        // 注意：箭头键会产生ESC序列(三个字节): 0x1B, 0x5B, 0x41/42/43/44
        int c = getchar();
        if (c == EOF) {
            break; // 读取出错或Ctrl+D等退出
        }

        if (c == 'q') {
            // 用户按'q'，退出摇杆模式
            break;
        }
        else if (c == '0') {
            // 按'0' -> 回中
            auto frame = createControlFrame(ACTION_GIMBAL_CENTER, {0});
            // 推送frame到发送队列，如:
            enqueueWithThrottle(frame);
            std::cout << "[Gimbal] center\n";
        }
        else if (c == 0x1B) {
            // 可能是箭头键或其他ESC序列
            int c1 = getchar(); // 一般是 0x5B
            int c2 = getchar(); // 具体 0x41=up, 0x42=down, 0x43=right, 0x44=left
            if (c1 == 0x5B) {
                DataFrame frame; // 创建一个空的DataFrame用于存储控制帧
                switch (c2) {
                case 0x41: {
                    // 上箭头
                    frame = createControlFrame(ACTION_GIMBAL_UP,{100}); // 这里假设上升100单位
                    std::cout << "[Gimbal] up\n";
                    break;
                }
                case 0x42: {
                    // 下箭头
                    frame = createControlFrame(ACTION_GIMBAL_DOWN,{100}); // 这里假设下降100单位
                    std::cout << "[Gimbal] down\n";
                    break;
                }
                case 0x43: {
                    // 右箭头
                    frame = createControlFrame(ACTION_GIMBAL_RIGHT,{100}); // 这里假设右移100单位
                    std::cout << "[Gimbal] right\n";
                    break;
                }
                case 0x44: {
                    // 左箭头
                    frame = createControlFrame(ACTION_GIMBAL_LEFT,{100}); // 这里假设左移100单位
                    std::cout << "[Gimbal] left\n";
                    break;
                }
                default:
                    // 其他ESC序列，不处理
                    break;
                }
                if(!frame.empty()) // 确保frame非空
                {
                    // std::lock_guard<std::mutex> lk(g_queueMutex);
                    // g_dataFrameQueue.push(frame);
                    enqueueWithThrottle(frame);
                }
                // 唤醒发送线程
                g_queueCond.notify_one();
            }
        }
        else {
            // 其它按键，不做处理
        }
    }

    // 恢复终端属性
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << "Exiting gimbal joystick mode...\n";
}
