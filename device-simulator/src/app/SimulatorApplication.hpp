#pragma once

/**
 * @file   SimulatorApplication.hpp
 * @brief  设备模拟器应用：模拟 IEC 104 子站（迭代 2 起实现协议栈）
 */

namespace simulator {

/**
 * @brief 模拟器进程入口，周期上送遥测并响应遥控
 */
class SimulatorApplication {
public:
    SimulatorApplication();
    ~SimulatorApplication();

    int run();
    void requestStop();

private:
    SimulatorApplication(const SimulatorApplication&);
    SimulatorApplication& operator=(const SimulatorApplication&);

    bool running_;
};

}  // namespace simulator
