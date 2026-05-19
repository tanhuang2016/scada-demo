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
#include "scada/protocol.hpp"
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
              << "  IEC104 监听端口: " << scada::config::kDeviceToMasterPort << '\n';

    scada::Telemetry sample;
    sample.deviceId = "RTU001";
    sample.voltage = 220.0;
    sample.current = 10.0;
    sample.switchState = scada::SwitchState::Closed;
    sample.timestamp = 0;

    /* 迭代 2：改为 104 服务器；此处仅演示文本编码 */
    std::cout << "  示例帧: " << scada::protocol::encodeTelemetry(sample) << '\n';

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
