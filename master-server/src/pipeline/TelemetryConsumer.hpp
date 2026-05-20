#pragma once

/**
 * @file   TelemetryConsumer.hpp
 * @brief  业务层遥测消费：控制台打印 + 推 Qt + 跟踪每设备在线状态
 * @module master-server
 */

#include <map>
#include <string>

#include "scada/types.hpp"

namespace master {
namespace push {
class UiBroadcaster;
}

namespace pipeline {

/**
 * @brief 遥测消费者——业务层入口
 *
 * 业务层只依赖 Telemetry 结构，不感知 JSON / 104 帧格式。
 *
 * 迭代 4：多设备并发采集。跟踪每设备在线状态：
 *   - 首次收到遥测 → 发送 ONLINE|deviceCode 帧到 Qt
 *   - 设备断线 → 发送 OFFLINE|deviceCode 帧到 Qt
 *   - 多次调用 onDeviceOffline / onTelemetry 会去重
 */
class TelemetryConsumer {
public:
    TelemetryConsumer();

    /** @brief 设置主站→Qt 推送器 */
    void setBroadcaster(push::UiBroadcaster* broadcaster);

    /** @brief 收到一条遥测数据（多设备线程安全） */
    void onTelemetry(const scada::Telemetry& telemetry);

    /** @brief 设备离线通知（发送 OFFLINE 帧到 Qt） */
    void onDeviceOffline(const std::string& deviceCode);

    /** @brief 报告协议未实现 */
    static void reportNotImplemented(const std::string& deviceCode, const char* protocolName);

private:
    push::UiBroadcaster* broadcaster_;
    std::map<std::string, bool> onlineMap_;  ///< deviceCode → 是否在线
};

}  // namespace pipeline
}  // namespace master
