/**
 * @file   SimulatorApplication.cpp
 * @brief  设备模拟器实现（脚手架：打印示例 TELEM 帧）
 */

#include "app/SimulatorApplication.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <windows.h>

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
    // 设置控制台输出为UTF-8编码
    SetConsoleOutputCP(65001);  // 65001是UTF-8的代码页

    running_ = true;
    std::cout << "[device-simulator] 启动（脚手架）\n"
              << "  JSON 设备端口: " << scada::config::kDeviceJsonPort << '\n'
              << "  (IEC104 预留端口: " << scada::config::kIec104ReservedPort << ")\n";

    scada::Telemetry sample;
    sample.deviceId = "RTU001";
    sample.voltage = 220.0;
    sample.current = 10.0;
    sample.switchState = scada::SwitchState::Closed;
    sample.timestamp = 0;

    scada::device_protocol::JsonProtocol jsonProto;
    std::string frame;
    if (jsonProto.encodeTelemetry(sample, frame)) {
        std::cout << "  示例 JSON 帧: " << frame;
    }

    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[device-simulator] 已停止\n";
    return 0;
}

void SimulatorApplication::requestStop()
{
    running_ = false;
}

}  // namespace simulator
