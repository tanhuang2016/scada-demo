#pragma once

/**
 * @file   TelemetryConsumer.hpp
 * @brief  业务层遥测消费：接收 Telemetry 并处理（本迭代：控制台打印）
 * @module master-server
 */

#include <string>

#include "scada/types.hpp"

namespace master {
namespace pipeline {

/**
 * @brief 遥测消费者——业务层入口
 *
 * 业务层只依赖 Telemetry 结构，不感知 JSON / 104 帧格式。
 * 本迭代仅做控制台输出；后续迭代在此处入库、推 UI、判告警。
 */
class TelemetryConsumer {
public:
    /** @brief 收到一条遥测数据 */
    void onTelemetry(const scada::Telemetry& telemetry);

    /** @brief 设备离线通知 */
    void onDeviceOffline(const std::string& deviceCode);

    /**
     * @brief 报告协议未实现（IEC104 占位设备）
     * @param deviceCode   设备编码
     * @param protocolName 协议名称
     */
    static void reportNotImplemented(const std::string& deviceCode, const char* protocolName);
};

}  // namespace pipeline
}  // namespace master
