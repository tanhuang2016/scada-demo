#pragma once

/**
 * @file   CommandRouter.hpp
 * @brief  遥控命令路由：接收 Qt CTRL 帧 → 路由到设备 → 写操作日志
 * @module master-server
 */

#include <string>
#include <vector>
#include <mutex>

#include "scada/device_protocol.hpp"
#include "storage/DeviceRepository.hpp"

namespace master {
namespace net {
class DeviceConnector;
}

namespace pipeline {

/**
 * @brief 遥控命令路由器
 *
 * 维护在线设备的连接器注册表。
 * 收到 CTRL 命令时：
 *   1. 查找目标设备的 DeviceConnector
 *   2. 通过 IDeviceProtocol::encodeControl 生成设备帧
 *   3. 通过 DeviceConnector::sendLine 发送
 *   4. 写入 MySQL operation_log
 *   5. 返回 SUCCESS/FAILURE
 */
class CommandRouter {
public:
    CommandRouter();
    ~CommandRouter();

    /** @brief 注册设备（设备线程启动时调用） */
    void registerDevice(const storage::DeviceConfig& cfg,
                        net::DeviceConnector* connector,
                        scada::device_protocol::IDeviceProtocol* protocol);

    /** @brief 注销设备（设备线程退出时调用） */
    void unregisterDevice(const std::string& deviceCode);

    /**
     * @brief 处理遥控命令
     * @param deviceCode  目标设备编码
     * @param switchVal   1=合闸, 0=分闸
     * @return "SUCCESS" 或 "FAILURE"
     */
    std::string handleCtrl(const std::string& deviceCode, int switchVal);

private:
    /** @brief 记录操作日志到 MySQL */
    bool logOperation(const std::string& deviceCode, int deviceId,
                      const std::string& opType, const std::string& result,
                      int beforeVal, int afterVal, const std::string& reason);

    struct DeviceEntry {
        storage::DeviceConfig config;
        net::DeviceConnector* connector;
        scada::device_protocol::IDeviceProtocol* protocol;
    };

    std::vector<DeviceEntry> devices_;
    std::mutex mutex_;
};

}  // namespace pipeline
}  // namespace master
