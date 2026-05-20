#pragma once

/**
 * @file   protocol.hpp
 * @brief  主站 ↔ Qt 文本行协议（与设备侧 JSON 分离）
 *
 * 设备侧协议见 device_protocol.hpp / JsonProtocol / Iec104Protocol。
 */

#include <string>

#include "scada/types.hpp"

namespace scada {
namespace protocol {

/**
 * @brief 将遥测结构编码为 TELEM 文本行（脚手架/调试用）
 */
std::string encodeTelemetry(const Telemetry& telemetry);

/**
 * @brief 解析 TELEM 文本行
 * @param line  输入报文
 * @param out   成功时写入解析结果
 * @return      是否解析成功
 */
bool decodeTelemetry(const std::string& line, Telemetry& out);

/**
 * @brief 根据默认电压限值判断是否告警
 */
bool isVoltageAlarm(double voltage);

}  // namespace protocol
}  // namespace scada
