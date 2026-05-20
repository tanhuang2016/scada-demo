#pragma once

/**
 * @file   TelemetryConsumer.hpp
 * @brief  业务层遥测消费：接收 Telemetry → 控制台打印 + 推送 Qt
 * @module master-server
 */

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
 * 本迭代：控制台输出 + 经 UiBroadcaster 推送 Qt 客户端。
 * 后续迭代在此处入库、判告警。
 */
class TelemetryConsumer {
public:
    TelemetryConsumer();

    /**
     * @brief 设置主站→Qt 推送器（可选，不设置则不推送）
     * @param broadcaster  UiBroadcaster 指针（由 MasterApplication 持有所有权）
     */
    void setBroadcaster(push::UiBroadcaster* broadcaster);

    /** @brief 收到一条遥测数据 */
    void onTelemetry(const scada::Telemetry& telemetry);

    /** @brief 设备离线通知（同时通知 Qt 客户端） */
    void onDeviceOffline(const std::string& deviceCode);

    /**
     * @brief 报告协议未实现（IEC104 占位设备）
     */
    static void reportNotImplemented(const std::string& deviceCode, const char* protocolName);

private:
    push::UiBroadcaster* broadcaster_;  ///< 非拥有，由 MasterApplication 管理生命周期
};

}  // namespace pipeline
}  // namespace master
