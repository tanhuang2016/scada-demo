#pragma once

/**
 * @file   AlarmEngine.hpp
 * @brief  告警引擎——电压越限/通信中断判定、去重、持久化到 MySQL
 * @module master-server
 */

#include <map>
#include <mutex>
#include <string>

#include "scada/types.hpp"

namespace master {
namespace pipeline {

/**
 * @brief 告警引擎
 *
 * 在 TelemetryConsumer 中被调用，对每条遥测做告警判定。
 * 抑制重复告警：同一设备同一类型只记录一条 ACTIVE 告警。
 * 告警恢复时（电压回归正常/设备重新上线）自动写入 CLEARED 状态。
 */
class AlarmEngine {
public:
    AlarmEngine();

    /**
     * @brief 处理一条遥测，判定电压告警
     * @param telemetry  遥测数据
     * @return 修改后的 telemetry（alarm 字段被设置）
     */
    void checkTelemetry(scada::Telemetry& telemetry);

    /**
     * @brief 设备离线告警
     * @param deviceCode  设备编码
     */
    void onDeviceOffline(const std::string& deviceCode);

    /**
     * @brief 设备上线，清除通信中断告警
     * @param deviceCode  设备编码
     */
    void onDeviceOnline(const std::string& deviceCode);

private:
    /** @brief 写入告警记录到 MySQL */
    void insertAlarm(const std::string& deviceCode, const std::string& type,
                     const std::string& title, double value, double limitHigh, double limitLow);

    /** @brief 清除之前的 ACTIVE 告警 */
    void clearAlarm(const std::string& deviceCode, const std::string& type);

    struct DeviceAlarmState {
        bool voltageHigh;   ///< 电压越上限
        bool voltageLow;    ///< 电压越下限
        bool commLost;      ///< 通信中断
        DeviceAlarmState() : voltageHigh(false), voltageLow(false), commLost(false) {}
    };

    std::map<std::string, DeviceAlarmState> states_;
    std::mutex mutex_;  ///< 保护 states_ 的多线程访问
};

}  // namespace pipeline
}  // namespace master
