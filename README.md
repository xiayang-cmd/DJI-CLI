# DJI-CLI ⟡ Ground-Control Station for DJI PSDK

轻量级、跨平台的 **CLI + ImGui** 地面站，面向运行 DJI Payload SDK (PSDK) 固件的无人机。  
既提供 *Unix-风格命令行* 的精准控制，又内嵌极简 **HUD** 实时显示遥测数据。

<p align="center">
  <img src="docs/screenshot_hud.png" width="640" alt="HUD Screenshot">
</p>

---

## ✨ 特性一览
| 类别 | 说明 |
|------|------|
| **控制接口** | ⚙️ 完整的命令行解析器，覆盖起飞 / 降落 / RTH / 航线飞行 / 云台 / 相机 / 避障等常用动作 |
| **通信协议** | ✉️ 支持心跳帧、注册帧、自定义控制帧；基于 `FrameAssembler` / `ReplyFrameDecoder` 可靠收发 |
| **航线规划** | 📍 JSON ↔︎ Protobuf 双向转换 (`RouteDataModule`)；示例航线位于 `config/planData.json` |
| **遥测可视化** | 📊 ImGui-HUD (`TelemetryUI`) 每帧解码 Protobuf 结构体并即时刷新 |
| **可扩展** | 🔌 各子模块（任务线程、命令解析、UI、协议）解耦，便于二次开发与移植 |

---

## 🗄️ 目录结构
```text
├── config/                 # JSON 配置 / 航线
├── tasks/                  # 线程 / 协议相关模块
│   ├── modules/            # 解析、封装、UI 等
│   └── utils/              # 网络通信、摇杆示例
├── third_party/            # Protobuf 生成代码等
├── common/                 # 公共工具 & 类型
├── vendor/imgui/           # ImGui 源码（已内置，无需额外下载）
├── CMakeLists.txt
└── main.cpp

---

## 🛠️ 构建与依赖

| 依赖 | 版本 | 说明 |
|------|------|------|
| **CMake** | ≥ 3.16 | 构建系统 |
| **C++17** | — | g++ 11 / clang 14 / MSVC 17 测试通过 |
| **Protobuf** | ≥ 3.15 | `protoc` + `libprotobuf` |
| **GLFW 3** | — | ImGui 后端 |
| **OpenGL** | — | 渲染 |
| **nlohmann/json** | 已内置 | 解析 / pretty-print |
| **ImGui** | 已内置 | GUI |

### Linux / macOS

```bash
# 1. 安装依赖（Ubuntu/Debian 示例）
sudo apt update
sudo apt install build-essential cmake libprotobuf-dev protobuf-compiler \
                 libglfw3-dev libgl1-mesa-dev

# 2. 编译
git clone https://github.com/xiayang-cmd/DJI-CLI.git
cd dji-cli
cmake -B build
cmake --build build -j$(nproc)

# 3. 运行
./build/dji-cli                 # 或 ./build/dji-cli path/to/config.json
