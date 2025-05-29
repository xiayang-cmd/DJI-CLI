// GimbalJoystickController.h

#ifndef GIMBAL_JOYSTICK_CONTROLLER_H
#define GIMBAL_JOYSTICK_CONTROLLER_H

#include <vector>
#include <cstdint>
#include "common_types.h"

// 你的项目中 DataFrame 的定义
using DataFrame = std::vector<uint8_t>;


/**
 * @brief 用于云台“摇杆控制”的类。进入run()后，捕获键盘输入并生成相应的数据帧。
 */
class GimbalJoystickController
{
public:
    /**
     * @brief 进入摇杆控制模式，直到用户按'q'退出
     */
    void run();
};

#endif // GIMBAL_JOYSTICK_CONTROLLER_H
