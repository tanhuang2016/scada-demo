/**
 * @file   SimulatorApplication.cpp
 * @brief  设备模拟器实现：TCP 5001 监听 + 周期 JSON 遥测上送
 * @module device-simulator
 */

#include "app/SimulatorApplication.hpp"

#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iostream>
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

    std::cout << "[device-simulator] 启动\n"
              << "  JSON 设备端口: " << scada::config::kDeviceJsonPort << "\n"
              << "  (IEC104 预留端口: " << scada::config::kIec104ReservedPort << ")\n";

    /* 启动 TCP 服务端 */
    net::JsonTcpServer server(scada::config::kDeviceJsonPort);
    if (!server.start()) {
        std::cerr << "[device-simulator] TCP 服务启动失败\n";
        return 1;
    }

    /* 准备模拟遥测数据 */
    scada::device_protocol::JsonProtocol jsonProto;
    scada::Telemetry telem;
    telem.deviceId = "RTU001";
    telem.switchState = scada::SwitchState::Closed;

    std::cout << "[device-simulator] 等待主站连接...\n";

    while (running_) {
        /* 等待主站连接（1秒超时，便于检查 running_） */
        if (!server.hasClient()) {
            server.waitForClient(1000);
            continue;
        }

        /* 生成带随机波动的模拟遥测 */
        telem.voltage = 220.0 + (std::rand() % 200 - 100) / 10.0;   // 210~230V
        telem.current = 10.0 + (std::rand() % 100 - 50) / 10.0;     // 5~15A
        telem.timestamp = static_cast<std::int64_t>(std::time(NULL));

        /* 通过 JsonProtocol 编码，不手动拼 JSON */
        std::string frame;
        if (jsonProto.encodeTelemetry(telem, frame)) {
            if (!server.sendLine(frame)) {
                std::cerr << "[device-simulator] 发送失败，客户端可能已断开\n";
                server.closeClient();
                std::cout << "[device-simulator] 等待主站连接...\n";
                continue;
            }
            std::cout << "[device-simulator] >> " << frame;
        }

        /* 1 秒间隔上送，分片 sleep 以响应 requestStop */
        for (int i = 0; i < 10 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    server.stop();
    std::cout << "[device-simulator] 已停止\n";
    return 0;
}

void SimulatorApplication::requestStop()
{
    running_ = false;
}

}  // namespace simulator
