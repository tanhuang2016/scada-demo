/**
 * @file   MasterApplication.cpp
 * @brief  主站应用实现（迭代5：热加载 + ControlListener 控制通道）
 * @module master-server
 */

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "app/MasterApplication.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "net/ControlListener.hpp"
#include "net/DeviceConnector.hpp"
#include "pipeline/TelemetryConsumer.hpp"
#include "push/UiBroadcaster.hpp"
#include "scada/config_defaults.hpp"
#include "scada/protocol_factory.hpp"
#include "storage/DeviceRepository.hpp"

#ifndef USE_MOCK_STORAGE
#include "storage/MySQLConnection.hpp"
#endif

namespace master {

MasterApplication::MasterApplication()
    : running_(false)
{
}

MasterApplication::~MasterApplication()
{
}

int MasterApplication::run()
{
    running_ = true;
    std::cout << "[master-server] 启动\n"
              << "  UI 推送端口: " << scada::config::kMasterToUiPort << "\n"
              << "  UI 控制端口: " << scada::config::kUiToMasterPort << "\n";

#ifdef USE_MOCK_STORAGE
    std::cout << "  配置模式: 模拟（无 MySQL 依赖）\n";
#else
    storage::MySQLConfig mysqlConfig;
    mysqlConfig.host = "127.0.0.1";
    mysqlConfig.port = 3306;
    mysqlConfig.username = "root";
    mysqlConfig.password = "tanhuang";
    mysqlConfig.database = "scada_demo";

    storage::MySQLConnection::instance().initialize(mysqlConfig);

    bool mysqlOk = storage::MySQLConnection::instance().isValid();
    std::cout << "  配置模式: 真实 MySQL (连接状态: " << (mysqlOk ? "OK" : "FAILED") << ")\n";
#endif

    /* 启动长生命周期服务 */
    push::UiBroadcaster broadcaster(scada::config::kMasterToUiPort);
    broadcaster.start();

    net::ControlListener controlListener(scada::config::kUiToMasterPort);
    controlListener.start(&running_);

    bool permanentStop = false;

    /*
     * 热加载主循环：
     *   1. 从 MySQL 加载设备配置
     *   2. 启动设备连接线程
     *   3. 等待 running_ == false（信号触发或 RELOAD 命令触发）
     *   4. 如果是 RELOAD → 重新加载配置 → 回到步骤 2
     *   5. 如果是 SIGINT → 退出
     */
    while (!permanentStop) {
        running_ = true;
        storage::DeviceRepository repo;
        pipeline::TelemetryConsumer consumer;
        consumer.setBroadcaster(&broadcaster);
        std::vector<std::thread> deviceThreads;

        /*
         * 启动所有 JSON 设备的连接线程。
         * 每次热加载都会重新创建线程，旧线程在上一轮 join 后已结束。
         */
        int count = startAllDeviceThreads(repo, consumer, deviceThreads);
        if (count == 0) {
            std::cerr << "[master-server] 无可连接的设备\n";
            permanentStop = true;
            break;
        }

        std::cout << "[master-server] 共 " << count
                  << " 台设备线程运行中\n";

        /* 等待所有设备线程退出 */
        for (std::vector<std::thread>::iterator it = deviceThreads.begin();
             it != deviceThreads.end(); ++it) {
            if (it->joinable()) it->join();
        }

        /* 判断退出原因 */
        if (controlListener.reloadRequested()) {
            controlListener.clearReload();
            std::cout << "\n[master-server] === 热加载：重新读取 MySQL 配置 ===\n\n";
            /* running_ 在下一次循环开始时重置为 true */
        } else {
            permanentStop = true;
        }
    }

    controlListener.stop();
    broadcaster.stop();
    std::cout << "[master-server] 已停止\n";
    return 0;
}

/*
 * 从 MySQL 加载设备配置并为每台可连接设备启动线程。
 * 返回启动的线程数。
 */
int MasterApplication::startAllDeviceThreads(
    storage::DeviceRepository& repo,
    pipeline::TelemetryConsumer& consumer,
    std::vector<std::thread>& threads)
{
    std::vector<storage::DeviceConfig> devices;
    if (!repo.loadAllEnabled(devices) || devices.empty()) {
        return 0;
    }

    std::cout << "\n[master-server] 加载 " << devices.size() << " 台设备配置\n";
    for (std::vector<storage::DeviceConfig>::const_iterator it = devices.begin();
         it != devices.end(); ++it) {
        std::cout << "  [" << it->deviceCode << "] " << it->deviceName
                  << "  → " << it->ipAddress << ":" << it->port
                  << " (" << it->protocol << ")\n";
    }
    std::cout << std::endl;

    int count = 0;
    for (std::vector<storage::DeviceConfig>::iterator it = devices.begin();
         it != devices.end(); ++it) {
        scada::device_protocol::ProtocolKind kind;
        std::unique_ptr<scada::device_protocol::IDeviceProtocol> proto =
            scada::device_protocol::createProtocolFromName(it->protocol, kind);

        if (!proto->isImplemented()) {
            pipeline::TelemetryConsumer::reportNotImplemented(it->deviceCode, proto->name());
            continue;
        }

        std::cout << "[master-server] 启动设备线程: " << it->deviceCode
                  << " (" << it->ipAddress << ":" << it->port << ")\n";

        threads.push_back(std::thread(
            runDeviceThread, *it, std::move(proto), &consumer, &running_));
        ++count;
    }
    return count;
}

void MasterApplication::runDeviceThread(
    const storage::DeviceConfig& config,
    std::unique_ptr<scada::device_protocol::IDeviceProtocol> protocol,
    pipeline::TelemetryConsumer* consumer,
    bool* running)
{
    net::DeviceConnector connector(config, std::move(protocol));

    while (*running) {
        std::cout << "[" << config.deviceCode << "] 连接 "
                  << config.ipAddress << ":" << config.port << "...\n";

        if (!connector.connect()) {
            if (!*running) break;
            for (int i = 0; i < config.reconnectIntervalSec * 10 && *running; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        std::cout << "[" << config.deviceCode << "] 已连接\n";

        std::string line;
        int noDataCount = 0;
        const int kMaxNoData = 4;

        while (*running && connector.isConnected()) {
            if (connector.readLine(line, 500)) {
                noDataCount = 0;
                scada::Telemetry telem;
                if (connector.protocol()->decodeTelemetry(line, telem)) {
                    consumer->onTelemetry(telem);
                }
            } else {
                if (!connector.isConnected()) break;
                ++noDataCount;
                if (noDataCount >= kMaxNoData) {
                    std::cout << "[" << config.deviceCode << "] "
                              << (kMaxNoData * 500 / 1000) << " 秒无数据，判定离线\n";
                    connector.disconnect();
                    break;
                }
            }
        }

        if (connector.isConnected()) connector.disconnect();
        consumer->onDeviceOffline(config.deviceCode);

        if (*running) {
            for (int i = 0; i < config.reconnectIntervalSec * 10 && *running; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

void MasterApplication::requestStop()
{
    running_ = false;
}

}  // namespace master
