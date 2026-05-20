#pragma once

/**
 * @file   DeviceConnector.hpp
 * @brief  主站 TCP 客户端：连接设备（模拟器），按行读取并经 IDeviceProtocol 解码
 * @module master-server
 */

#include <memory>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "scada/device_protocol.hpp"
#include "storage/DeviceRepository.hpp"

namespace master {
namespace net {

/**
 * @brief 设备连接器：管理到单台设备的 TCP 连接，按行读取帧并使用 IDeviceProtocol 解码
 *
 * 业务层通过 readLine + protocol()->decodeTelemetry 获取 Telemetry，
 * 不直接拼/解析 JSON 或 104 帧。
 */
class DeviceConnector {
public:
    /**
     * @param config   设备配置（IP、端口、重连间隔等）
     * @param protocol 由工厂创建的协议实例（JsonProtocol / Iec104Protocol）
     */
    DeviceConnector(const storage::DeviceConfig& config,
                    std::unique_ptr<scada::device_protocol::IDeviceProtocol> protocol);

    ~DeviceConnector();

    /** @brief 建立 TCP 连接 */
    bool connect();

    /** @brief 断开连接 */
    void disconnect();

    /** @brief 当前是否已连接 */
    bool isConnected() const;

    /**
     * @brief 发送一行数据到设备（线程安全，用于遥控等下行命令）
     * @return 发送成功与否
     */
    bool sendLine(const std::string& line);

    /**
     * @brief 读取一行帧数据（带超时）
     * @param line      输出：读取到的一行（不含 \r\n）
     * @param timeoutMs select 超时毫秒，0 表示阻塞
     * @return true 读取到完整一行；false 超时或断连，用 isConnected() 区分
     */
    bool readLine(std::string& line, int timeoutMs = 1000);

    /** @brief 获取协议实例（用于 decodeTelemetry） */
    scada::device_protocol::IDeviceProtocol* protocol() const;

    /** @brief 获取设备配置 */
    const storage::DeviceConfig& config() const;

private:
    storage::DeviceConfig config_;
    std::unique_ptr<scada::device_protocol::IDeviceProtocol> protocol_;
    SOCKET sock_;
    bool connected_;
    std::mutex sendMutex_;    ///< 保护 send() 调用，允许多线程同时发送
    std::string recvBuffer_;  ///< 接收缓冲区，用于按行切分
};

}  // namespace net
}  // namespace master
