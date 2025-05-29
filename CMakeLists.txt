cmake_minimum_required(VERSION 3.16)
project(DJI-CLI)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找并引入 Protobuf，如果需要自动编译 .proto 文件，请参考后文“如需自动编译 .proto 文件”的说明
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

# ── ImGui（新增）───────────────────────────────────────────────────────
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)

# 收集 ImGui 源文件 + 后端桥接
file(GLOB IMGUI_SRC
     ${IMGUI_DIR}/*.cpp
     ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
     ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)

add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends)

# 如用 GLFW/OpenGL 后端，则还要找 GLFW & OpenGL
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# ── 业务源码 ────────────────────────────────────────────────────────────
# 指定源文件
set(SOURCE_FILES
    main.cpp
    tasks/TasksManager.cpp
    tasks/com_task.cpp
    tasks/utils/CLinuxTCPCom.cpp
    tasks/utils/GimbalJoystickController.cpp
    tasks/modules/CLI2Frame.cpp
    tasks/modules/FrameAssembler.cpp
    tasks/modules/ReplyFrameDecoder.cpp
    tasks/modules/FrameDataHandler.cpp
    tasks/modules/TelemetryUI.cpp
    tasks/modules/routeDataModule.cpp
    third_party/protobuf/TelemetryDataBuf-new.pb.cpp
    common/common_types.cpp
    common/common_utils.cpp
)

# 生成可执行文件
add_executable(dji-cli ${SOURCE_FILES})

# 指定头文件搜索路径
target_include_directories(dji-cli PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf
    ${CMAKE_CURRENT_SOURCE_DIR}/common        # 如果 common 目录中有需要暴露的头文件，可自行调整
)

# 链接所需的库
target_link_libraries(dji-cli
    imgui
    pthread
    rt
    protobuf
    dl
)