#pragma once

/**
 * @file   SimulatorApplication.hpp
 * @brief  设备模拟器应用：TCP 服务端 + 周期 JSON 遥测上送
 * @module device-simulator
 */

namespace simulator {

/**
 * @brief 模拟器进程入口，模拟 1 台 RTU 设备周期上送遥测
 */
class SimulatorApplication {
public:
    SimulatorApplication();
    ~SimulatorApplication();

    /**
     * @brief 启动模拟器逻辑，阻塞直到 requestStop
     * @return 进程退出码
     */
    int run();

    /** @brief 请求结束主循环 */
    void requestStop();

private:
    SimulatorApplication(const SimulatorApplication&);
    SimulatorApplication& operator=(const SimulatorApplication&);

    bool running_;
};

}  // namespace simulator
