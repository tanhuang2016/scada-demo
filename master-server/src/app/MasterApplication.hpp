#pragma once

/**
 * @file   MasterApplication.hpp
 * @brief  主站应用：生命周期与后台服务编排（迭代 2 起 TCP 采集）
 * @module master-server
 */

#include <string>
#include <vector>

#include "scada/device_protocol.hpp"
#include "storage/DeviceRepository.hpp"

namespace master {
namespace net {
class DeviceConnector;
}

namespace pipeline {
class TelemetryConsumer;
}

/**
 * @brief 主站进程入口类，负责 run 循环与优雅退出
 */
class MasterApplication {
public:
    MasterApplication();
    ~MasterApplication();

    /**
     * @brief 启动主站逻辑，阻塞直到 requestStop
     * @return 进程退出码，0 表示正常
     */
    int run();

    /** @brief 请求结束主循环（信号处理或外部调用） */
    void requestStop();

private:
    MasterApplication(const MasterApplication&);
    MasterApplication& operator=(const MasterApplication&);

    /**
     * @brief 连接设备并进入遥测读取循环
     * @param connector  设备连接器
     * @param consumer   遥测消费者
     * @return 0 正常退出，1 连接失败
     */
    int runDeviceLoop(net::DeviceConnector& connector, pipeline::TelemetryConsumer& consumer);

    bool running_;
};

}  // namespace master
