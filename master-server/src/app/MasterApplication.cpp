/**
 * @file   MasterApplication.cpp
 * @brief  主站应用实现（迭代2：JSON 单设备遥测采集）
 * @module master-server
 */

/* winsock2.h 必须在 windows.h / mysql.h 之前包含 */
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

#include "net/DeviceConnector.hpp"
#include "pipeline/TelemetryConsumer.hpp"
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
              << "  JSON 设备端口: " << scada::config::kDeviceJsonPort << "\n"
              << "  UI 推送端口: " << scada::config::kMasterToUiPort << "\n"
              << "  UI 控制端口: " << scada::config::kUiToMasterPort << "\n";

#ifdef USE_MOCK_STORAGE
    std::cout << "  配置模式: 模拟（无 MySQL 依赖）\n";
#else
    /* 初始化 MySQL 连接 */
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
    std::cout << "------------------------------------------------------------\n";
    for (const auto& device : devices) {
        std::cout << "  [" << device.deviceCode << "] " << device.deviceName << "\n"
                  << "      通信: " << device.ipAddress << ":" << device.port
                  << " (" << device.protocol << ")\n";
    }
    std::cout << "------------------------------------------------------------\n";

    /* 遍历设备，按 protocol 字段选协议，找到首个可连接的 JSON 设备 */
    storage::DeviceConfig targetDevice;
    std::unique_ptr<scada::device_protocol::IDeviceProtocol> targetProtocol;
    bool foundTarget = false;

    for (std::vector<storage::DeviceConfig>::iterator it = devices.begin();
         it != devices.end(); ++it) {
        scada::device_protocol::ProtocolKind kind;
        std::unique_ptr<scada::device_protocol::IDeviceProtocol> proto =
            scada::device_protocol::createProtocolFromName(it->protocol, kind);

        if (!proto->isImplemented()) {
            /* IEC104 等未实现协议：打印提示，不崩溃 */
            pipeline::TelemetryConsumer::reportNotImplemented(it->deviceCode, proto->name());
            continue;
        }

        if (!foundTarget) {
            /* 迭代2：仅连接第一台 JSON 设备 */
            targetDevice = *it;
            targetProtocol = std::move(proto);
            foundTarget = true;
            std::cout << "[master-server] 目标设备: " << targetDevice.deviceCode
                      << " (" << targetDevice.ipAddress << ":" << targetDevice.port << ")\n";
        }
    }

    if (!foundTarget) {
        std::cerr << "[master-server] 无可连接的设备（所有协议均未实现）\n";
        return 1;
    }

    /* 创建连接器和消费者，进入遥测读取循环 */
    net::DeviceConnector connector(targetDevice, std::move(targetProtocol));
    pipeline::TelemetryConsumer consumer;

    int ret = runDeviceLoop(connector, consumer);

    connector.disconnect();
    std::cout << "[master-server] 已停止\n";
    return ret;
}

int MasterApplication::runDeviceLoop(net::DeviceConnector& connector,
                                     pipeline::TelemetryConsumer& consumer)
{
    const storage::DeviceConfig& cfg = connector.config();

    while (running_) {
        /* 尝试连接 */
        std::cout << "[master-server] 连接 " << cfg.deviceCode
                  << " (" << cfg.ipAddress << ":" << cfg.port << ")...\n";

        if (!connector.connect()) {
            if (!running_) break;
            /* 连接失败，等待重连间隔 */
            std::cout << "[master-server] 等待 " << cfg.reconnectIntervalSec
                      << " 秒后重连...\n";
            for (int i = 0; i < cfg.reconnectIntervalSec * 10 && running_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        std::cout << "[master-server] 连接成功，开始接收遥测\n";

        /* 遥测读取循环：select 超时计数器作兜底断连检测 */
        std::string line;
        int noDataCount = 0;
        const int kMaxNoData = 4;   ///< 500ms * 4 = 2秒无数据视为断连

        while (running_ && connector.isConnected()) {
            if (connector.readLine(line, 500)) {
                noDataCount = 0;  // 有数据到达，重置
                /* 通过 IDeviceProtocol 解码，业务层不直接拼 JSON */
                scada::Telemetry telem;
                if (connector.protocol()->decodeTelemetry(line, telem)) {
                    consumer.onTelemetry(telem);
                } else {
                    std::cerr << "[master-server] 遥测解析失败: " << line << "\n";
                }
            } else {
                /* readLine 返回 false：可能是超时，也可能是断连 */
                if (!connector.isConnected()) {
                    break;  // 已明确断连
                }
                /* select 超时，累计超时次数作兜底 */
                ++noDataCount;
                if (noDataCount >= kMaxNoData) {
                    std::cout << "[master-server] " << (kMaxNoData * 500 / 1000)
                              << " 秒无数据，判定离线\n";
                    connector.disconnect();
                    break;
                }
            }
        }

        /* 连接断开 */
        if (connector.isConnected()) {
            connector.disconnect();
        }
        consumer.onDeviceOffline(cfg.deviceCode);

        if (running_) {
            std::cout << "[master-server] 等待 " << cfg.reconnectIntervalSec
                      << " 秒后重连...\n";
            for (int i = 0; i < cfg.reconnectIntervalSec * 10 && running_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    return 0;
}

void MasterApplication::requestStop()
{
    running_ = false;
}

}  // namespace master
