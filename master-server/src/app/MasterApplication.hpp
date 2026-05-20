#pragma once

/**
 * @file   MasterApplication.hpp
 * @brief  主站应用：多设备并发采集 + UiBroadcaster + ControlListener 热加载
 * @module master-server
 */

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "scada/device_protocol.hpp"
#include "storage/DeviceRepository.hpp"

namespace master {
namespace net {
class ControlListener;
class DeviceConnector;
}
namespace pipeline {
class CommandRouter;
class TelemetryConsumer;
}
namespace push {
class UiBroadcaster;
}

class MasterApplication {
public:
    MasterApplication();
    ~MasterApplication();

    int run();
    void requestStop();

private:
    static void runDeviceThread(const storage::DeviceConfig& config,
                                std::unique_ptr<scada::device_protocol::IDeviceProtocol> protocol,
                                pipeline::TelemetryConsumer* consumer,
                                pipeline::CommandRouter* router,
                                bool* running);

    /** @brief 加载设备配置并启动连接线程，返回启动的线程数 */
    int startAllDeviceThreads(storage::DeviceRepository& repo,
                              pipeline::TelemetryConsumer& consumer,
                              pipeline::CommandRouter& router,
                              std::vector<std::thread>& threads);

    MasterApplication(const MasterApplication&);
    MasterApplication& operator=(const MasterApplication&);

    bool running_;
};

}  // namespace master
