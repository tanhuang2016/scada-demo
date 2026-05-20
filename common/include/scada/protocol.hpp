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
 * @deprecated 设备侧以 JSON 为准，主站→Qt 请用 encodeUpdate
 */
std::string encodeTelemetry(const Telemetry& telemetry);

/**
 * @brief 解析 TELEM 文本行
 * @deprecated 同 encodeTelemetry
 */
bool decodeTelemetry(const std::string& line, Telemetry& out);

/**
 * @brief 根据默认电压限值判断是否告警
 */
bool isVoltageAlarm(double voltage);

// ========== 主站 → Qt 推送协议（UPDATE 帧） ==========

/**
 * @brief 将遥测编码为 UPDATE 文本行（主站 → Qt 5002 通道）
 * @param telem 遥测快照
 * @return      UPDATE|deviceId|voltage|current|switch|timestamp
 *
 * 格式示例：UPDATE|RTU001|228.5|12.3|1|1716000000
 * switch: 1=合闸(Closed), 0=分闸(Open)
 */
std::string encodeUpdate(const Telemetry& telem);

/**
 * @brief 解析 UPDATE 文本行
 * @param line  输入报文，如 UPDATE|RTU001|228.5|12.3|1|1716000000
 * @param out   成功时写入解析结果
 * @return      解析是否成功
 */
bool decodeUpdate(const std::string& line, Telemetry& out);

}  // namespace protocol
}  // namespace scada
