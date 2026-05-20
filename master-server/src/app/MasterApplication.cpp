/**
 * @file   MasterApplication.cpp
 * @brief  主站应用实现（迭代4：多设备并发采集 + UiBroadcaster 推送 + 在线状态）
 *
 * 架构变化（迭代3→4）：
 *   - 迭代3：主线程只连接第一台 JSON 设备
 *   - 迭代4：每台 JSON 设备一个独立线程，并发连接/读取/重连
 *   - 共享 TelemetryConsumer + UiBroadcaster（线程安全）
 *   - 消费者跟踪每设备在线状态，发送 ONLINE/OFFLINE 帧到 Qt
 *
 * @module master-server
 */

/* winsock2.h 必须在 mysql.h 之前包含 */
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "app/MasterApplication.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

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

    /* 加载设备配置 */
    storage::DeviceRepository repo;
    std::vector<storage::DeviceConfig> devices;

    if (!repo.loadAllEnabled(devices) || devices.empty()) {
        std::cerr << "[master-server] 设备配置加载失败或无设备\n";
        return 1;
    }

    std::cout << "\n[master-server] 设备配置加载成功，共 " << devices.size() << " 台设备\n";
    for (std::vector<storage::DeviceConfig>::const_iterator it = devices.begin();
         it != devices.end(); ++it) {
        std::cout << "  [" << it->deviceCode << "] " << it->deviceName
                  << "  → " << it->ipAddress << ":" << it->port
                  << " (" << it->protocol << ")\n";
    }
    std::cout << std::endl;

    /* 启动 UiBroadcaster */
    push::UiBroadcaster broadcaster(scada::config::kMasterToUiPort);
    if (!broadcaster.start()) {
        std::cerr << "[master-server] UiBroadcaster 启动失败\n";
    }

    /* 创建遥测消费者（多个设备线程共享） */
    pipeline::TelemetryConsumer consumer;
    consumer.setBroadcaster(&broadcaster);

    /*
     * 为每台已实现的 JSON 设备启动独立连接线程。
     * 每台设备拥有独立的 DeviceConnector，共享 TelemetryConsumer。
     */
    std::vector<std::thread> deviceThreads;

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

        /*
         * detach 线程：设备线程独立运行 connect/read/reconnect 循环。
         * 通过 &running_ 检查退出信号。主线程 joining 时等待。
         */
        deviceThreads.push_back(std::thread(
            runDeviceThread, *it, std::move(proto), &consumer, &running_));
    }

    if (deviceThreads.empty()) {
        std::cerr << "[master-server] 无可连接的设备（所有协议均未实现）\n";
        broadcaster.stop();
        return 1;
    }

    std::cout << "[master-server] 共 " << deviceThreads.size()
              << " 台设备线程运行中，按 Ctrl+C 停止\n";

    /* 等待所有设备线程退出 */
    for (std::vector<std::thread>::iterator it = deviceThreads.begin();
         it != deviceThreads.end(); ++it) {
        if (it->joinable()) {
            it->join();
        }
    }

    broadcaster.stop();
    std::cout << "[master-server] 已停止\n";
    return 0;
}

/*
 * 单设备连接线程函数（静态成员，与旧版 runDeviceLoop 逻辑完全相同）。
 *
 * 离线检测三层策略（与 DeviceConnector::readLine 配合）：
 *   [第1层] select exceptfds — Windows 远端 RST
 *   [第2层] SO_ERROR — select 超时后显式检查
 *   [第3层] kMaxNoData 连续超时计数器 — 2 秒无数据判定离线
 *
 * 每台设备独立运行此函数。遥测通过共享 consumer 分发。
 */
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
            std::cout << "[" << config.deviceCode << "] 等待 "
                      << config.reconnectIntervalSec << " 秒后重连...\n";
            for (int i = 0; i < config.reconnectIntervalSec * 10 && *running; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        std::cout << "[" << config.deviceCode << "] 已连接\n";

        std::string line;
        int noDataCount = 0;
        const int kMaxNoData = 4;  // 500ms * 4 = 2s

        while (*running && connector.isConnected()) {
            if (connector.readLine(line, 500)) {
                noDataCount = 0;
                scada::Telemetry telem;
                if (connector.protocol()->decodeTelemetry(line, telem)) {
                    consumer->onTelemetry(telem);
                } else {
                    std::cerr << "[" << config.deviceCode << "] 解析失败: " << line << "\n";
                }
            } else {
                if (!connector.isConnected()) break;
                ++noDataCount;
                if (noDataCount >= kMaxNoData) {
                    std::cout << "[" << config.deviceCode << "] "
                              << (kMaxNoData * 500 / 1000)
                              << " 秒无数据，判定离线\n";
                    connector.disconnect();
                    break;
                }
            }
        }

        if (connector.isConnected()) connector.disconnect();
        consumer->onDeviceOffline(config.deviceCode);

        if (*running) {
            std::cout << "[" << config.deviceCode << "] 等待 "
                      << config.reconnectIntervalSec << " 秒后重连...\n";
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
