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

// ========== 主站 → Qt 设备状态通知 ==========

/**
 * @brief 编码设备离线通知
 * @return OFFLINE|deviceCode
 */
std::string encodeOffline(const std::string& deviceCode);

/**
 * @brief 编码设备上线通知
 * @return ONLINE|deviceCode
 */
std::string encodeOnline(const std::string& deviceCode);

/**
 * @brief 解析离线通知帧
 * @param line       输入，如 OFFLINE|RTU001
 * @param deviceCode 输出：设备编码
 * @return 是否解析成功
 */
bool decodeOffline(const std::string& line, std::string& deviceCode);

bool decodeOnline(const std::string& line, std::string& deviceCode);

// ========== 主站 ↔ 遥控协议（CTRL 帧） ==========

/**
 * @brief 编码遥控帧
 * @param deviceCode  设备编码，如 RTU001
 * @param switchVal   1=合闸, 0=分闸
 * @return CTRL|deviceCode|switchVal
 */
std::string encodeCtrl(const std::string& deviceCode, int switchVal);

/**
 * @brief 解析遥控帧
 * @param line        输入，如 CTRL|RTU001|1
 * @param deviceCode  输出：设备编码
 * @param switchVal   输出：1=合闸, 0=分闸
 * @return 是否解析成功
 */
bool decodeCtrl(const std::string& line, std::string& deviceCode, int& switchVal);

/**
 * @brief 编码遥控响应帧
 * @return CTRL_ACK|deviceCode|switchVal|SUCCESS 或 FAILURE
 */
std::string encodeCtrlAck(const std::string& deviceCode, int switchVal, bool success);

}  // namespace protocol
}  // namespace scada
