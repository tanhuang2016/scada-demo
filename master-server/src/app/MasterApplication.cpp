/**
 * @file   MasterApplication.cpp
 * @brief  主站应用实现（迭代2：JSON 单设备遥测采集）
 *
 * 流程概述：
 *   1. 加载 MySQL 设备配置
 *   2. 遍历设备，通过 protocol_factory 创建对应协议实例
 *   3. IEC104 设备：打印「未实现」然后跳过（不崩溃）
 *   4. 第一台 JSON 设备：TCP 连接 → 遥测读取循环 → 重连循环
 *
 * 离线检测三层策略（与 DeviceConnector::readLine 配合）：
 *   [第1层] select() exceptfds — Windows 上远端 RST 可能只走此路径
 *   [第2层] SO_ERROR — select 超时后显式检查
 *   [第3层] kMaxNoData 超时计数器 — 本文件的兜底，累计 2 秒无数据主动断开
 *
 * 为什么需要第 3 层：
 *   Windows 127.0.0.1 loopback 上 kill 对端进程后，
 *   select 可能既不置位 readfds 也不置位 exceptfds，只持续超时。
 *   DeviceConnector::readLine 的前两层策略在此场景下可能均不触发，
 *   因此在本层用连续超时计数器兜底。详见 DeviceConnector.cpp 注释。
 *
 * @module master-server
 */

/* winsock2.h 必须在 mysql.h（间接 include windows.h）之前包含 */
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

    /* 加载设备配置（如有 USE_MOCK_STORAGE 宏则走模拟路径，否则走真实 MySQL） */
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

    /*
     * 遍历全部启用设备，按 device.protocol 字段通过工厂创建协议实例。
     *
     * 关键约束（见 docs/architecture.md 和 docs/coding-standards.md）：
     *   - 业务层只通过 IDeviceProtocol 抽象接口操作，不判断具体协议类型
     *   - 不直接拼/解析 JSON 字符串或 104 帧
     *   - IEC104 设备仅打印提示并跳过，不崩溃
     *
     * 当前迭代（迭代2）仅连接第一台 JSON 设备；
     * 多设备并发将在迭代4实现。
     */
    storage::DeviceConfig targetDevice;
    std::unique_ptr<scada::device_protocol::IDeviceProtocol> targetProtocol;
    bool foundTarget = false;

    for (std::vector<storage::DeviceConfig>::iterator it = devices.begin();
         it != devices.end(); ++it) {
        scada::device_protocol::ProtocolKind kind;
        std::unique_ptr<scada::device_protocol::IDeviceProtocol> proto =
            scada::device_protocol::createProtocolFromName(it->protocol, kind);

        if (!proto->isImplemented()) {
            /* IEC104 占位实现：isImplemented() 为 false，打印日志后安全跳过 */
            pipeline::TelemetryConsumer::reportNotImplemented(it->deviceCode, proto->name());
            continue;
        }

        if (!foundTarget) {
            /* 找到第一台已实现的 JSON 设备，作为目标设备 */
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

    /*
     * 创建连接器和消费者，进入遥测读取循环。
     *
     * 连接器持有 IDeviceProtocol 实例和 TCP 套接字；
     * 消费者负责 Telemetry 的业务处理（本迭代仅控制台打印，后续入库/推 UI）。
     */
    net::DeviceConnector connector(targetDevice, std::move(targetProtocol));
    pipeline::TelemetryConsumer consumer;

    int ret = runDeviceLoop(connector, consumer);

    connector.disconnect();
    std::cout << "[master-server] 已停止\n";
    return ret;
}

/*
 * 遥测读取主循环：连接 → 读遥测 → 断线 → 重连。
 *
 * 离线检测三层策略（与 DeviceConnector::readLine 配合，由深到浅）：
 *   [第1层] DeviceConnector::readLine 内部 select exceptfds
 *           — Windows 远端 RST 优先走此路径
 *   [第2层] DeviceConnector::readLine 内部 select 超时后 getsockopt(SO_ERROR)
 *           — select 持续超时但连接已断（Windows loopback 常见）
 *   [第3层] 本函数的 kMaxNoData 连续超时计数器
 *           — 以上两层均未捕获的极端情况，累计 2 秒无数据主动断开
 *
 * 模拟器约每秒发送一帧，正常情况下 noDataCount 不会超过 2。
 * 如果模拟器阻塞、断连但 TCP 层未及时通知等，计数器会在 2 秒后触发。
 *
 * sleep 分片为 100ms 间隔：既能及时响应 requestStop()（由 SIGINT 触发），
 * 又避免忙等消耗 CPU。
 */
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
            /*
             * 连接失败，等待后重连。
             * 常见错误码参考：
             *   10061 (WSAECONNREFUSED)：模拟器未启动 / 端口未监听
             *   10060 (WSAETIMEDOUT)：网络不通
             *   10054 (WSAECONNRESET)：中间设备重置连接
             */
            std::cout << "[master-server] 等待 " << cfg.reconnectIntervalSec
                      << " 秒后重连...\n";
            for (int i = 0; i < cfg.reconnectIntervalSec * 10 && running_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        std::cout << "[master-server] 连接成功，开始接收遥测\n";

        /*
         * 遥测读取循环。
         *
         * readLine(line, 500) 每 500ms 超时一次。
         * - 返回 true：收到完整一行 → 解码 Telemetry → 消费
         * - 返回 false + connected_=true：超时 → 累计 noDataCount
         * - 返回 false + connected_=false：断连 → 退出内层循环
         *
         * kMaxNoData = 4 → 4 × 500ms = 2 秒无数据判定离线。
         * 模拟器约 1 秒发一帧，正常不会触发。
         *
         * noDataCount 在收到数据时重置为 0（心跳复位）。
         */
        std::string line;
        int noDataCount = 0;
        const int kMaxNoData = 4;   ///< 500ms * 4 = 2秒无数据视为断连

        while (running_ && connector.isConnected()) {
            if (connector.readLine(line, 500)) {
                noDataCount = 0;  // 有数据到达，重置超时计数器（心跳复位）
                /* 通过 IDeviceProtocol 解码，业务层不直接拼 JSON */
                scada::Telemetry telem;
                if (connector.protocol()->decodeTelemetry(line, telem)) {
                    consumer.onTelemetry(telem);
                } else {
                    std::cerr << "[master-server] 遥测解析失败: " << line << "\n";
                }
            } else {
                /*
                 * readLine 返回 false：两种可能
                 * 1. select 超时，连接正常 → 累计计数器，检查是否达到 kMaxNoData
                 * 2. 连接断开（select 异常或 recv 失败）→ isConnected() 为 false → break
                 */
                if (!connector.isConnected()) {
                    break;  // 已通过 readLine 内部三层策略检测到断连
                }
                /* select 超时但 isConnected() = true → 累计无数据时间 */
                ++noDataCount;
                if (noDataCount >= kMaxNoData) {
                    /*
                     * 第 3 层兜底生效。
                     * 进入此分支说明前两层（exceptfds / SO_ERROR）均未触发，
                     * 但连续 2 秒未收到任何数据。
                     * 主动关闭连接，触发重连逻辑。
                     */
                    std::cout << "[master-server] " << (kMaxNoData * 500 / 1000)
                              << " 秒无数据，判定离线 (检测点: 第3层超时计数器)\n";
                    connector.disconnect();
                    break;
                }
            }
        }

        /* 连接断开（正常/异常关闭或第 3 层计数超时） */
        if (connector.isConnected()) {
            connector.disconnect();
        }
        consumer.onDeviceOffline(cfg.deviceCode);

        if (running_) {
            std::cout << "[master-server] 等待 " << cfg.reconnectIntervalSec
                      << " 秒后重连...\n";
            /* 分片 sleep：100ms 一片，能及时响应 requestStop() */
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
