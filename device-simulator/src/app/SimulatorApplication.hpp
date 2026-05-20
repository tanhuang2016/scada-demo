#pragma once

/**
 * @file   SimulatorApplication.hpp
 * @brief  设备模拟器应用：3 台设备独立 TCP 服务端 + 周期 JSON 遥测上送
 * @module device-simulator
 */

#include <string>
#include <thread>
#include <vector>

namespace simulator {

/**
 * @brief 模拟器进程入口，模拟 3 台 RTU 设备并发周期上送遥测
 *
 * 每台设备独立 TCP 端口 + 独立线程：
 *   RTU001 → 5001, RTU002 → 5011, RTU003 → 5012
 */
class SimulatorApplication {
public:
    SimulatorApplication();
    ~SimulatorApplication();

    int run();
    void requestStop();

private:
    struct DeviceThread;
    void runDeviceThread(DeviceThread& dt);

    SimulatorApplication(const SimulatorApplication&);
    SimulatorApplication& operator=(const SimulatorApplication&);

    bool running_;
};

}  // namespace simulator
