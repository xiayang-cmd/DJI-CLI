#include "TelemetryUI.h"
#include <iostream>
#include <chrono>
#include <thread>

// GLFW + OpenGL + ImGui 相关头
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

TelemetryUI::TelemetryUI()
{
    // 构造函数可以做一些初始化（如果需要的话）
}

TelemetryUI::~TelemetryUI()
{
    // 确保在析构时停止线程
    stop();
}

void TelemetryUI::start()
{
    // 如果线程已经在跑，则不重复启动
    if (m_thread.joinable()) {
        return;
    }

    m_stop = false;
    m_thread = std::thread(&TelemetryUI::uiThreadFunc, this);
}

void TelemetryUI::stop()
{
    m_stop = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void TelemetryUI::update(const TelemetryData& data)
{
    // 用锁保护，防止渲染线程同时访问
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_data = data;
}

void TelemetryUI::updateUavState(const UavState& state)
{
    std::lock_guard<std::mutex> lock(m_uavStateMutex);
    m_uavState = state;
}

void TelemetryUI::uiThreadFunc()
{
    // ---------------------------
    // 1) 初始化 GLFW
    // ---------------------------
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW.\n";
        return;
    }
    // 简单起见，用最基本的 OpenGL 2.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 700, "Telemetry UI", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // ---------------------------
    // 2) 初始化 ImGui
    // ---------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    (void)io;
    ImGui::StyleColorsDark();

    // 如果你要使用现代的 OpenGL core profile，可以把 "#version 120" 改成 "#version 330 core"
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 120");

    // ---------------------------
    // 3) 主循环
    // ---------------------------
    while (!glfwWindowShouldClose(window) && !m_stop)
    {
        glfwPollEvents();

        // 新 ImGui 帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 渲染 Telemetry 信息
        render();

        // ImGui 收尾并绘制
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // ---------------------------
    // 4) 清理
    // ---------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void TelemetryUI::render()
{
    // -----------------------------
    // 先显示 TelemetryData 的信息
    // -----------------------------
    {
        // 在一个自动布局的窗口中显示 m_data
        // 可以根据项目需要把显示做得更复杂和美观
        std::lock_guard<std::mutex> lock(m_dataMutex);

        ImGui::Begin("TelemetryData", nullptr,
                    ImGuiWindowFlags_NoResize      |
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoCollapse);

        // 示例：每个字段显示一行
        ImGui::Text("lng: %.6f", m_data.lng());
        ImGui::Text("lat: %.6f", m_data.lat());
        ImGui::Text("altitude: %.2f m", m_data.altitude());
        ImGui::Text("ultrasonic: %.2f m", m_data.ultrasonic());
        ImGui::Text("pitch: %.2f deg", m_data.pitch());
        ImGui::Text("roll: %.2f deg", m_data.roll());
        ImGui::Text("yaw: %.2f deg", m_data.yaw());
        ImGui::Text("airspeed: %.2f m/s", m_data.airspeed());
        ImGui::Text("velocity: %.2f m/s", m_data.velocity());
        ImGui::Text("timestamp: %llu", static_cast<unsigned long long>(m_data.timestamp()));
        ImGui::Text("ptpitch: %.2f deg", m_data.ptpitch());
        ImGui::Text("ptroll: %.2f deg", m_data.ptroll());
        ImGui::Text("ptyaw: %.2f deg", m_data.ptyaw());
        ImGui::Text("zoomfactor: %.2f x", m_data.zoomfactor());
        ImGui::Text("boxSn: %s", m_data.boxsn().c_str());
        ImGui::Text("batteryPower: %s", m_data.batterypower().c_str());
        ImGui::Text("satelliteCount: %u", m_data.satellitecount());
        ImGui::Text("taskId: %llu", static_cast<unsigned long long>(m_data.taskid()));
        ImGui::Text("rtkLng: %.6f", m_data.rtklng());
        ImGui::Text("rtkLat: %.6f", m_data.rtklat());
        ImGui::Text("rtkHFSL: %.2f m", m_data.rtkhfsl());
        ImGui::Text("rtkPositionInfo: %u", m_data.rtkpositioninfo());
        ImGui::Text("airFlyTimes: %u s", m_data.airflytimes());
        ImGui::Text("airFlyDistance: %.2f m", m_data.airflydistance());
        ImGui::Text("uavSn: %s", m_data.uavsn().c_str());
        ImGui::Text("uavModel: %s", m_data.uavmodel().c_str());
        ImGui::Text("homeRange: %.2f m", m_data.homerange());
        ImGui::Text("flightMode: %u", m_data.flightmode());
        ImGui::Text("targetDistance: %.2f m", m_data.targetdistance());
        ImGui::Text("predictFlyTime: %u s", m_data.predictflytime());
        ImGui::Text("ultrasonicMax: %.2f m", m_data.ultrasonicmax());
        ImGui::Text("ultrasonicMin: %.2f m", m_data.ultrasonicmin());
        ImGui::Text("xVelocity: %.2f m/s", m_data.xvelocity());
        ImGui::Text("yVelocity: %.2f m/s", m_data.yvelocity());
        ImGui::Text("zVelocity: %.2f m/s", m_data.zvelocity());
        ImGui::Text("boxName: %s", m_data.boxname().c_str());
        ImGui::Text("predictFlyTimes: %u s", m_data.predictflytimes());
        ImGui::Text("predictGohomeBattery: %u %%", m_data.predictgohomebattery());
    
        ImGui::End();
    }

    // -----------------------------
    // 新增：显示 UavState 的信息
    // 我们使用一个新的窗口 + TabBar 形式，把飞控、电池、云台、相机、任务、避障、HMS 分成若干 Tab
    // -----------------------------
    {
        // 为了保证读写安全，需要加锁
        std::lock_guard<std::mutex> lock(m_uavStateMutex);

        ImGui::Begin("UAV State",
                     nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoCollapse);

        // 使用 TabBar 把不同模块分开
        if (ImGui::BeginTabBar("UavStateTabBar"))
        {
            // ============ 1. 飞控(FlightControllerState) ============
            if (ImGui::BeginTabItem("FlightController"))
            {
                const auto& fcs = m_uavState.flightcontrollerstate();
                ImGui::Text("SatelliteCount: %u", fcs.satellitecount());
                ImGui::Text("GpsSignalLevel: %u", fcs.gpssignallevel());
                ImGui::Text("FlightMode: %u", fcs.flightmode());
                ImGui::Text("FlightStatus: %u", fcs.flightstatus());
                
                // startPoint
                const auto& sp = fcs.startpoint();
                ImGui::Text("StartPoint: (lat=%.6f, lng=%.6f, height=%.2f)", 
                            sp.lat(), sp.lng(), sp.height());
                
                // homePoint
                const auto& hp = fcs.homepoint();
                ImGui::Text("HomePoint: (lat=%.6f, lng=%.6f)", hp.lat(), hp.lng());

                ImGui::Text("HomeHeight: %d m", fcs.homeheight());
                ImGui::Text("SafeLine: %u", fcs.safeline());
                ImGui::Text("DeviceStatus: %u", fcs.devicestatus());
                ImGui::Text("UWB NodeCount: %u", fcs.uwbnodecount());
                ImGui::Text("RC Mode: %s", fcs.rcmode().c_str());
                ImGui::Text("isEmergencyBrakeing: %u", fcs.isemergencybrakeing());
                ImGui::Text("outFlyAreaFlag: %u", fcs.outflyareaflag());
                ImGui::Text("inNoFlyAreaFlag: %u", fcs.innoflyareaflag());

                const auto& tp = fcs.targetpoint();
                ImGui::Text("TargetPoint: (lat=%.6f, lng=%.6f, height=%.2f)",
                            tp.lat(), tp.lng(), tp.height());

                ImGui::Text("BatteryGoHome: %u %%", fcs.batterygohome());
                ImGui::Text("AutoLowGoHome: %u", fcs.autolowgohome());
                ImGui::Text("rcConnected: %u", fcs.rcconnected());

                ImGui::EndTabItem();
            }

            // ============ 2. 电池(BatteryState) ============
            if (ImGui::BeginTabItem("Battery"))
            {
                const auto& bs = m_uavState.batterystate();
                ImGui::Text("BatteryNum: %u", bs.batterynum());
                ImGui::Text("BatteryPower (%%): %s", bs.batterypower().c_str());
                ImGui::Text("BatteryVoltage (V): %s", bs.batteryvoltage().c_str());

                // 分别显示 2 块电池信息
                const auto& b1 = bs.firstbatteryinfo();
                ImGui::Separator();
                ImGui::Text("Battery #1:");
                ImGui::Text("  isBatteryEmbed: %u", b1.isbatteryembed());
                ImGui::Text("  capacityPercent: %u", b1.batterycapacitypercent());
                ImGui::Text("  currentVoltage: %d mV", b1.currentvoltage());
                ImGui::Text("  currentElectric: %d mA", b1.currentelectric());
                ImGui::Text("  fullCapacity: %u mAh", b1.fullcapacity());
                ImGui::Text("  remainedCapacity: %u mAh", b1.remainedcapacity());
                ImGui::Text("  batteryTemperature: %.1f C", b1.batterytemperature());
                ImGui::Text("  cellCount: %u", b1.cellcount());
                ImGui::Text("  batSOHState: %u", b1.batsohstate());
                ImGui::Text("  sop: %u", b1.sop());
                ImGui::Text("  heatState: %u", b1.heatstate());
                ImGui::Text("  socState: %u", b1.socstate());
                ImGui::Text("  selfCheckError: %u", b1.selfcheckerror());

                const auto& b2 = bs.secondbatteryinfo();
                ImGui::Separator();
                ImGui::Text("Battery #2:");
                ImGui::Text("  isBatteryEmbed: %u", b2.isbatteryembed());
                ImGui::Text("  capacityPercent: %u", b2.batterycapacitypercent());
                ImGui::Text("  currentVoltage: %d mV", b2.currentvoltage());
                ImGui::Text("  currentElectric: %d mA", b2.currentelectric());
                ImGui::Text("  fullCapacity: %u mAh", b2.fullcapacity());
                ImGui::Text("  remainedCapacity: %u mAh", b2.remainedcapacity());
                ImGui::Text("  batteryTemperature: %.1f C", b2.batterytemperature());
                ImGui::Text("  cellCount: %u", b2.cellcount());
                ImGui::Text("  batSOHState: %u", b2.batsohstate());
                ImGui::Text("  sop: %u", b2.sop());
                ImGui::Text("  heatState: %u", b2.heatstate());
                ImGui::Text("  socState: %u", b2.socstate());
                ImGui::Text("  selfCheckError: %u", b2.selfcheckerror());

                ImGui::EndTabItem();
            }

            // ============ 3. 云台(PtzState) ============
            if (ImGui::BeginTabItem("Gimbal"))
            {
                const auto& ptz = m_uavState.ptzstate();
                ImGui::Text("Pitch: %.2f deg", ptz.pitch());
                ImGui::Text("Roll:  %.2f deg", ptz.roll());
                ImGui::Text("Yaw:   %.2f deg", ptz.yaw());
                ImGui::Text("Mode:  %u", ptz.gimbalmode());  // 1=自由模式, 2=跟随, 3=FPV

                ImGui::EndTabItem();
            }

            // ============ 4. 相机(CameraState) ============
            if (ImGui::BeginTabItem("Camera"))
            {
                const auto& cs = m_uavState.camerastate();
                ImGui::Text("Mode: %u (1=拍照,2=录像)", cs.mode());
                ImGui::Text("isRecording: %u", cs.isrecording());
                ImGui::Text("recordDuration: %u s", cs.recordduration());
                ImGui::Text("Source: %u (0=可见,1=变焦,2=红外)", cs.source());
                ImGui::Text("Camera: %u (0=FPV,1=相机1,2=相机2)", cs.camera());
                ImGui::Text("ZoomFactor: %.2f x", cs.zoomfactor());
                ImGui::Text("Resolution: %u x %u", cs.width(), cs.height());
                ImGui::Text("FrameRate: %u", cs.framerate());
                ImGui::Text("Bitstream: %u (单位 0.001Mbps)", cs.bitstream());
                ImGui::Text("pointThermometrying: %u", cs.pointthermometrying());
                ImGui::Text("areaThermometrying: %u", cs.areathermometrying());
                ImGui::Text("laserRanging: %u", cs.laserranging());
                ImGui::Text("storePercent: %.2f %%", cs.storepercent());
                ImGui::Text("totalPhoto: %u", cs.totalphoto());
                ImGui::Text("remainedPhoto: %u", cs.remainedphoto());
                ImGui::Text("sendPhotoOverFlag: %u", cs.sendphotooverflag());

                ImGui::EndTabItem();
            }

            // ============ 5. 任务(MissionState) ============
            if (ImGui::BeginTabItem("Mission"))
            {
                const auto& ms = m_uavState.missionstate();
                ImGui::Text("isPause: %u", ms.ispause());
                ImGui::Text("targetWaypointIndex: %u", ms.targetwaypointindex());
                ImGui::Text("isWaypointFinished(废弃): %u", ms.iswaypointfinished());
                ImGui::Text("pushVideo: %u", ms.pushvideo());
                ImGui::Text("lock4g: %u", ms.lock4g());
                ImGui::Text("boxModel: %u", ms.boxmodel());
                ImGui::Text("mapPlay(interval): %u", ms.mapplay());
                ImGui::Text("loseAction: %u (0=返回HOME,1=继续航线)", ms.loseaction());
                ImGui::Text("isPointControl: %u", ms.ispointcontrol());
                ImGui::Text("isUwbLanding: %u", ms.isuwblanding());
                ImGui::Text("isPushVideoing: %u", ms.ispushvideoing());
                ImGui::Text("isDLanding: %u", ms.isdlanding());
                ImGui::Text("perceptionFlag: %u (0=载荷降落,1=下视降落)", ms.perceptionflag());
                ImGui::Text("isPositionControl: %u", ms.ispositioncontrol());
                ImGui::Text("sendImgRtp: %u", ms.sendimgrtp());
                
                // 航线断点信息
                const auto& bp = ms.breakpoint();
                if (bp.status() == 1) {
                    ImGui::Separator();
                    ImGui::Text("BreakPoint status: %u", bp.status());
                    ImGui::Text("  point: %u", bp.point());
                    ImGui::Text("  lng: %.6f", bp.lng());
                    ImGui::Text("  lat: %.6f", bp.lat());
                    ImGui::Text("  height: %.2f", bp.height());
                    ImGui::Text("  taskId: %llu", (unsigned long long)bp.taskid());
                } else {
                    ImGui::Text("No valid BreakPoint data");
                }

                ImGui::EndTabItem();
            }

            // ============ 6. 避障(AvoidanceData) ============
            if (ImGui::BeginTabItem("Avoidance"))
            {
                const auto& ad = m_uavState.avoidancedata();

                ImGui::Text("[Down]   dist=%.2f m,   health=%u", ad.down(),   ad.downhealth());
                ImGui::Text("[Up]     dist=%.2f m,   health=%u", ad.up(),     ad.uphealth());
                ImGui::Text("[Front]  dist=%.2f m,   health=%u", ad.front(),  ad.fronthealth());
                ImGui::Text("[Back]   dist=%.2f m,   health=%u", ad.back(),   ad.backhealth());
                ImGui::Text("[Left]   dist=%.2f m,   health=%u", ad.left(),   ad.lefthealth());
                ImGui::Text("[Right]  dist=%.2f m,  health=%u", ad.right(),  ad.righthealth());

                ImGui::Text("AvoidanceEnableStatus(Down): %u", ad.avoidanceenablestatusdown());
                ImGui::Text("AvoidanceEnableStatus(Up): %u", ad.avoidanceenablestatusup());
                ImGui::Text("AvoidanceEnableStatus(Horizontal): %u", ad.avoidanceenablestatushorizontal());

                ImGui::EndTabItem();
            }

            // ============ 7. HMS 报警信息(HmsAlarmData) ============
            if (ImGui::BeginTabItem("HMS Alarm"))
            {
                // m_uavState.hmsalarmdata() 是 repeated 的消息, 需要遍历
                const int alarmCount = m_uavState.hmsalarmdata_size();
                ImGui::Text("Alarm Count: %d", alarmCount);

                ImGui::Separator();
                for (int i = 0; i < alarmCount; ++i) {
                    const auto& alarm = m_uavState.hmsalarmdata(i);
                    ImGui::Text("Alarm #%d:", i);
                    ImGui::Text("  alarmId: %u", alarm.alarmid());
                    ImGui::Text("  reportLevel: %u", alarm.reportlevel());
                    ImGui::Text("  alarmMssInfo: %s", alarm.alarmmssinfo().c_str());
                    ImGui::Separator();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // 最后：显示 boxSn + timestamp
        ImGui::Separator();
        ImGui::Text("boxSn: %s", m_uavState.boxsn().c_str());
        ImGui::Text("timestamp: %llu", (unsigned long long)m_uavState.timestamp());

        ImGui::End(); // end UAV State window
    }
}
