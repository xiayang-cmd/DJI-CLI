#include "CLinuxTCPCom.h" // 引入 UDP 通信头文件
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <fstream> // 文件读写
#include <iostream>
#include <nlohmann/json.hpp> // 引入 JSON 库
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <atomic>
#include "common_types.h" // 引入公共类型定义
#include "common_utils.h" // 引入公共工具函数
#include "TasksManager.h"
#include "CLI2Frame.h"   // 引入命令行到帧的转换器

using namespace std;

int main(int argc, char* argv[])
{

    // 0. 获取服务器配置
    std::string cfgPath = "../config/config.json";

    // 如果用户在命令行提供了路径，则覆盖默认值
    if (argc > 1) {
        cfgPath = argv[1];
    }
 
    g_serverConfig = getServerConfig(cfgPath);

    // 1. 创建一个任务管理器
    TasksManager tasksMgr;

    // 2, 初始化（TCP连接、ComTask等）
    if(!tasksMgr.initAllTasks()){
        cerr << "任务管理器初始化失败，请检查配置文件或网络连接。" << endl;
        return -1; // 初始化失败，退出程序
    }

    // 3, 启动线程（收发数据）
    tasksMgr.startAllTasks();
    // 小睡一会儿，等待线程启动
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 4, 主线程CLI循环
    printf("\n");
    printf("欢迎使用 pxh 地面站 CLI!\n");
    std::cout << "CLI 地面站示例. 输入命令, 如: takeoff 10\n";
    std::cout << "输入 exit 退出.\n";

    while (true) {
        std::cout << "pxh> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            break; // 处理 Ctrl+D 等情况
        }
        if (line == "exit") {
            break;
        }
        // 解析并构建数据帧
        DataFrame frame = parseCommand(line);
        if(!frame.empty())
        {
            std::lock_guard<std::mutex> lk(g_queueMutex);
            g_dataFrameQueue.push(frame);
        }
        // 唤醒发送线程
        g_queueCond.notify_one();
    }

    return 0;
}