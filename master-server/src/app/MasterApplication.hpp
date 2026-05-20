#pragma once

/**
 * @file   MasterApplication.hpp
 * @brief  主站应用：多设备并发采集 + UiBroadcaster 推送 + 在线状态跟踪
 * @module master-server
 */

#include <string>
#include <thread>
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
namespace push {
class UiBroadcaster;
}

/**
 * @brief 主站进程入口类，负责多设备线程调度与优雅退出
 */
class MasterApplication {
public:
    MasterApplication();
    ~MasterApplication();

    int run();
    void requestStop();

private:
    /**
     * @brief 单设备连接线程：连接 → 读遥测 → 断线 → 重连
     *
     * 每台 JSON 设备启动一个线程运行此函数。
     * 共享 consumer 和 broadcaster（后者已内置互斥锁，线程安全）。
     */
    static void runDeviceThread(const storage::DeviceConfig& config,
                                std::unique_ptr<scada::device_protocol::IDeviceProtocol> protocol,
                                pipeline::TelemetryConsumer* consumer,
                                bool* running);

    MasterApplication(const MasterApplication&);
    MasterApplication& operator=(const MasterApplication&);

    bool running_;
};

}  // namespace master
