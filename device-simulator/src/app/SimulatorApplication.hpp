#pragma once

/**
 * @file   SimulatorApplication.hpp
 * @brief  设备模拟器：从 MySQL 动态加载设备列表 + 独立 TCP 服务端
 * @module device-simulator
 */

#include <string>
#include <thread>
#include <vector>

namespace simulator {

class SimulatorApplication {
public:
    SimulatorApplication();
    ~SimulatorApplication();

    int run();
    void requestStop();

private:
    struct DeviceThread;

    /** @brief 从 MySQL 加载启用设备列表，失败则用硬编码默认值 */
    int loadDevicesFromDb(std::vector<DeviceThread*>& out);

    /** @brief 单设备线程 */
    void runDeviceThread(DeviceThread& dt);

    SimulatorApplication(const SimulatorApplication&);
    SimulatorApplication& operator=(const SimulatorApplication&);

    bool running_;
};

}  // namespace simulator
