/**
 * @file   SimulatorApplication.cpp
 * @brief  设备模拟器实现：3 台设备独立 TCP 端口 + 独立线程并发遥测上送
 *
 * 每台设备：
 *   - 监听独立端口（RTU001/5001, RTU002/5011, RTU003/5012）
 *   - 独立线程运行 accept → send 循环
 *   - 电压/电流/开关数据各设备独立随机，互不干扰
 *
 * @module device-simulator
 */

#include "app/SimulatorApplication.hpp"

#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "net/JsonTcpServer.hpp"
#include "scada/config_defaults.hpp"
#include "scada/json_protocol.hpp"
#include "scada/types.hpp"

namespace simulator {

namespace {
    std::mutex g_randMutex;  ///< 保护 std::rand() 多线程调用
}  // namespace

/**
 * @brief 单设备模拟线程的上下文
 */
struct SimulatorApplication::DeviceThread {
    std::string deviceId;
    int port;
    int index;  ///< 设备序号 0/1/2，用于差异化随机数据
    std::string label;
    net::JsonTcpServer* server;   ///< 动态分配 — JsonTcpServer 无默认构造
    std::thread thread;
};

SimulatorApplication::SimulatorApplication()
    : running_(false)
{
}

SimulatorApplication::~SimulatorApplication()
{
}

int SimulatorApplication::run()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    running_ = true;
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    std::cout << "[device-simulator] 启动 — 模拟 3 台设备\n"
              << "  RTU001 → 端口 " << scada::config::kDeviceJsonPort << "\n"
              << "  RTU002 → 端口 " << scada::config::kDeviceJsonPort2 << "\n"
              << "  RTU003 → 端口 " << scada::config::kDeviceJsonPort3 << "\n";

    /* 配置 3 台设备（JsonTcpServer 需动态分配——无默认构造函数） */
    DeviceThread devices[3];
    devices[0].deviceId = "RTU001";
    devices[0].port = scada::config::kDeviceJsonPort;
    devices[0].index = 0;
    devices[0].label = "[RTU001:5001]";
    devices[0].server = new net::JsonTcpServer(scada::config::kDeviceJsonPort);

    devices[1].deviceId = "RTU002";
    devices[1].port = scada::config::kDeviceJsonPort2;
    devices[1].index = 1;
    devices[1].label = "[RTU002:5011]";
    devices[1].server = new net::JsonTcpServer(scada::config::kDeviceJsonPort2);

    devices[2].deviceId = "RTU003";
    devices[2].port = scada::config::kDeviceJsonPort3;
    devices[2].index = 2;
    devices[2].label = "[RTU003:5012]";
    devices[2].server = new net::JsonTcpServer(scada::config::kDeviceJsonPort3);

    /* 启动 3 台设备的 TCP 服务端 */
    for (int i = 0; i < 3; ++i) {
        if (!devices[i].server->start()) {
            std::cerr << "[device-simulator] " << devices[i].label
                      << " TCP 服务启动失败\n";
            running_ = false;
            /* 清理已启动的服务器 */
            for (int j = 0; j < i; ++j) {
                if (devices[j].thread.joinable()) devices[j].thread.join();
                delete devices[j].server;
            }
            return 1;
        }
    }

    /* 启动 3 台设备的工作线程 */
    for (int i = 0; i < 3; ++i) {
        devices[i].thread = std::thread(
            &SimulatorApplication::runDeviceThread, this,
            std::ref(devices[i]));
    }

    std::cout << "[device-simulator] 3 台设备全部启动，等待主站连接...\n";

    /* 等待所有设备线程退出 */
    for (int i = 0; i < 3; ++i) {
        if (devices[i].thread.joinable()) {
            devices[i].thread.join();
        }
    }

    /* 停止所有 TCP 服务端 */
    for (int i = 0; i < 3; ++i) {
        devices[i].server->stop();
        delete devices[i].server;
    }

    std::cout << "[device-simulator] 已停止\n";
    return 0;
}

/*
 * 单设备工作线程：等待主站连接 → 生成遥测 → 发送 → 睡眠循环。
 *
 * 每台设备独立运行此函数，互不干扰。
 * 延迟 100ms * deviceIndex 使设备启动错开，避免同时发送数据包。
 */
void SimulatorApplication::runDeviceThread(DeviceThread& dt)
{
    scada::device_protocol::JsonProtocol jsonProto;
    scada::Telemetry telem;
    telem.deviceId = dt.deviceId;
    telem.switchState = scada::SwitchState::Closed;

    std::cout << dt.label << " 线程启动\n";

    while (running_) {
        /* 等待主站连接 */
        if (!dt.server->hasClient()) {
            dt.server->waitForClient(1000);
            continue;
        }

        /* 生成随机遥测（std::rand 非线程安全，互斥锁保护）
         *
         * 每台设备按序号偏移基准值，使 3 组数据可分辨：
         *   RTU001(index=0): 基准 220V / 10A
         *   RTU002(index=1): 基准 222V / 12A
         *   RTU003(index=2): 基准 224V /  8A
         */
        {
            std::lock_guard<std::mutex> lock(g_randMutex);
            telem.voltage = (220.0 + dt.index * 2.0)
                          + (std::rand() % 200 - 100) / 10.0;
            telem.current = (10.0 + dt.index * 2.0 - (dt.index == 2 ? 4.0 : 0.0))
                          + (std::rand() % 100 - 50) / 10.0;
        }
        telem.timestamp = static_cast<std::int64_t>(std::time(NULL));

        std::string frame;
        if (jsonProto.encodeTelemetry(telem, frame)) {
            if (!dt.server->sendLine(frame)) {
                std::cerr << dt.label << " 发送失败，客户端可能已断开\n";
                dt.server->closeClient();
                continue;
            }
            std::cout << dt.label << " >> " << frame;
        }

        /* 1 秒间隔，分片检测 running_ */
        for (int i = 0; i < 10 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << dt.label << " 线程退出\n";
}

void SimulatorApplication::requestStop()
{
    running_ = false;
}

}  // namespace simulator
