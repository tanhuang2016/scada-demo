#pragma once

/**
 * @file   config_defaults.hpp
 * @brief  全系统默认端口、告警阈值等编译期常量
 */

namespace scada {
namespace config {

/** 设备模拟器 → 主站（IEC 104 默认端口，与协议栈配置一致） */
const int kDeviceToMasterPort = 2404;

/** 主站 → Qt 实时数据推送 */
const int kMasterToUiPort = 5002;

/** Qt → 主站（遥控、登录等） */
const int kUiToMasterPort = 5003;

/** 电压告警下限（伏特） */
const double kVoltageMin = 215.0;

/** 电压告警上限（伏特） */
const double kVoltageMax = 235.0;

}  // namespace config
}  // namespace scada
