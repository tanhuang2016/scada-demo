/**
 * @file   AlarmEngine.cpp
 * @brief  告警引擎实现
 * @module master-server
 */

#include "pipeline/AlarmEngine.hpp"

#include <iostream>
#include <sstream>

#include "scada/config_defaults.hpp"
#include "storage/MySQLConnection.hpp"

namespace master {
namespace pipeline {

AlarmEngine::AlarmEngine()
{
}

/*
 * 判定电压告警：
 *   - voltage > kVoltageMax (235V) → VOLTAGE_HIGH
 *   - voltage < kVoltageMin (215V) → VOLTAGE_LOW
 *
 * 去重策略：
 *   如果该设备该类型已有 ACTIVE 告警，不再重复写入。
 *   如果电压恢复正常，将对应的 ACTIVE 告警标记为 CLEARED。
 *
 * 设置 telemetry.alarm = true 以通知 Qt 显示红色。
 */
void AlarmEngine::checkTelemetry(scada::Telemetry& telemetry)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string& dev = telemetry.deviceId;
    double v = telemetry.voltage;
    bool hasAlarm = false;

    /* 电压越上限 */
    if (v > scada::config::kVoltageMax) {
        hasAlarm = true;
        if (!states_[dev].voltageHigh) {
            states_[dev].voltageHigh = true;
            insertAlarm(dev, "VOLTAGE_HIGH", "电压越上限",
                        v, scada::config::kVoltageMax, scada::config::kVoltageMin);
            std::cout << "[告警] " << dev << " 电压越上限: "
                      << v << "V > " << scada::config::kVoltageMax << "V\n";
        }
    } else if (states_[dev].voltageHigh) {
        states_[dev].voltageHigh = false;
        clearAlarm(dev, "VOLTAGE_HIGH");
        std::cout << "[告警] " << dev << " 电压越上限恢复\n";
    }

    /* 电压越下限 */
    if (v < scada::config::kVoltageMin) {
        hasAlarm = true;
        if (!states_[dev].voltageLow) {
            states_[dev].voltageLow = true;
            insertAlarm(dev, "VOLTAGE_LOW", "电压越下限",
                        v, scada::config::kVoltageMax, scada::config::kVoltageMin);
            std::cout << "[告警] " << dev << " 电压越下限: "
                      << v << "V < " << scada::config::kVoltageMin << "V\n";
        }
    } else if (states_[dev].voltageLow) {
        states_[dev].voltageLow = false;
        clearAlarm(dev, "VOLTAGE_LOW");
        std::cout << "[告警] " << dev << " 电压越下限恢复\n";
    }

    /* 通信中断也算告警 */
    if (states_[dev].commLost) {
        hasAlarm = true;
    }

    telemetry.alarm = hasAlarm;
}

void AlarmEngine::onDeviceOffline(const std::string& deviceCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!states_[deviceCode].commLost) {
        states_[deviceCode].commLost = true;
        insertAlarm(deviceCode, "COMM_LOST", "通信中断", 0.0, 0.0, 0.0);
        std::cout << "[告警] " << deviceCode << " 通信中断\n";
    }
}

void AlarmEngine::onDeviceOnline(const std::string& deviceCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (states_[deviceCode].commLost) {
        states_[deviceCode].commLost = false;
        clearAlarm(deviceCode, "COMM_LOST");
        std::cout << "[告警] " << deviceCode << " 通信恢复\n";
    }
}

/*
 * 写入一条 ACTIVE 告警到 MySQL alarm 表。
 */
void AlarmEngine::insertAlarm(const std::string& deviceCode, const std::string& type,
                               const std::string& title, double value,
                               double limitHigh, double limitLow)
{
    std::ostringstream sql;
    sql << "INSERT INTO alarm (device_id, device_code, alarm_type, alarm_level, "
        << "title, description, value_current, limit_high, limit_low, "
        << "state, occurred_at) VALUES ("
        << "0,"  // device_id 从 device_code 冗余
        << "'" << deviceCode << "',"
        << "'" << type << "',"
        << "'WARNING',"
        << "'" << title << "',"
        << "'" << title << "',"
        << value << ","
        << limitHigh << ","
        << limitLow << ","
        << "'ACTIVE',"
        << "NOW())";

    storage::MySQLConnection::instance().execute(sql.str());
}

/*
 * 将该设备该类型的所有 ACTIVE 告警标记为 CLEARED。
 */
void AlarmEngine::clearAlarm(const std::string& deviceCode, const std::string& type)
{
    std::ostringstream sql;
    sql << "UPDATE alarm SET state='CLEARED', cleared_at=NOW() "
        << "WHERE device_code='" << deviceCode << "'"
        << " AND alarm_type='" << type << "'"
        << " AND state='ACTIVE'";
    storage::MySQLConnection::instance().execute(sql.str());
}

}  // namespace pipeline
}  // namespace master
