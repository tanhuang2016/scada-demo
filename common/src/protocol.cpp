/**
 * @file   protocol.cpp
 * @brief  TELEM 文本行协议实现（C++11）
 */

#include "scada/protocol.hpp"

#include <sstream>

#include "scada/config_defaults.hpp"

namespace scada {
namespace protocol {

namespace {

const char kDelimiter = '|';
const char kTelemPrefix[] = "TELEM";

}  // namespace

std::string encodeTelemetry(const Telemetry& telemetry)
{
    std::ostringstream oss;
    oss << kTelemPrefix << kDelimiter << telemetry.deviceId << kDelimiter << telemetry.voltage
        << kDelimiter << telemetry.current << kDelimiter
        << static_cast<int>(telemetry.switchState) << kDelimiter << telemetry.timestamp;
    return oss.str();
}

bool decodeTelemetry(const std::string& line, Telemetry& out)
{
    /* 前缀必须为 TELEM| */
    if (line.size() < 6 || line.compare(0, 5, kTelemPrefix) != 0 || line[5] != kDelimiter) {
        return false;
    }

    Telemetry telemetry;
    char tag[16] = {};
    int switchState = 0;
    std::istringstream iss(line);
    char delimiter = 0;

    if (!(iss >> tag >> delimiter) || delimiter != kDelimiter) {
        return false;
    }
    if (!(std::getline(iss, telemetry.deviceId, kDelimiter) && !telemetry.deviceId.empty())) {
        return false;
    }
    if (!(iss >> telemetry.voltage >> delimiter) || delimiter != kDelimiter) {
        return false;
    }
    if (!(iss >> telemetry.current >> delimiter) || delimiter != kDelimiter) {
        return false;
    }
    if (!(iss >> switchState >> delimiter) || delimiter != kDelimiter) {
        return false;
    }
    if (!(iss >> telemetry.timestamp)) {
        return false;
    }

    telemetry.switchState = (switchState != 0) ? SwitchState::Closed : SwitchState::Open;
    telemetry.alarm = isVoltageAlarm(telemetry.voltage);
    out = telemetry;
    return true;
}

bool isVoltageAlarm(double voltage)
{
    return voltage < config::kVoltageMin || voltage > config::kVoltageMax;
}

// ========== 主站 → Qt 推送协议（UPDATE 帧） ==========

namespace {

const char kUpdatePrefix[] = "UPDATE";

}  // namespace

/**
 * 格式：UPDATE|deviceId|voltage|current|switch|timestamp|alarm
 * alarm: 0=正常, 1=告警
 */
std::string encodeUpdate(const Telemetry& telem)
{
    std::ostringstream oss;
    oss << kUpdatePrefix << kDelimiter << telem.deviceId << kDelimiter
        << telem.voltage << kDelimiter << telem.current << kDelimiter
        << static_cast<int>(telem.switchState) << kDelimiter << telem.timestamp
        << kDelimiter << (telem.alarm ? 1 : 0);
    return oss.str();
}

/**
 * 解析 UPDATE 帧。
 * 格式：UPDATE|deviceId|voltage|current|switch|timestamp|alarm
 */
bool decodeUpdate(const std::string& line, Telemetry& out)
{
    if (line.size() < 7 || line.compare(0, 6, kUpdatePrefix) != 0 || line[6] != kDelimiter) {
        return false;
    }

    Telemetry telemetry;
    std::istringstream iss(line);
    std::string token;
    int switchVal = 0;

    if (!std::getline(iss, token, kDelimiter)) return false;  // UPDATE
    if (!std::getline(iss, telemetry.deviceId, kDelimiter) || telemetry.deviceId.empty()) return false;
    if (!std::getline(iss, token, kDelimiter)) return false;
    telemetry.voltage = std::atof(token.c_str());
    if (!std::getline(iss, token, kDelimiter)) return false;
    telemetry.current = std::atof(token.c_str());
    if (!std::getline(iss, token, kDelimiter)) return false;
    switchVal = std::atoi(token.c_str());
    if (!std::getline(iss, token, kDelimiter)) return false;
    telemetry.timestamp = static_cast<std::int64_t>(std::atoll(token.c_str()));
    /* alarm 字段（第 7 列，可选——兼容旧版无此字段的帧） */
    if (std::getline(iss, token)) {
        telemetry.alarm = (std::atoi(token.c_str()) != 0);
    }

    telemetry.switchState = (switchVal != 0) ? SwitchState::Closed : SwitchState::Open;
    out = telemetry;
    return true;
}

// ========== 主站 → Qt 设备状态通知 ==========

namespace {

const char kOfflinePrefix[] = "OFFLINE";
const char kOnlinePrefix[] = "ONLINE";

}  // namespace

std::string encodeOffline(const std::string& deviceCode)
{
    std::ostringstream oss;
    oss << kOfflinePrefix << kDelimiter << deviceCode;
    return oss.str();
}

std::string encodeOnline(const std::string& deviceCode)
{
    std::ostringstream oss;
    oss << kOnlinePrefix << kDelimiter << deviceCode;
    return oss.str();
}

bool decodeOffline(const std::string& line, std::string& deviceCode)
{
    if (line.size() < 9 || line.compare(0, 7, kOfflinePrefix) != 0 || line[7] != kDelimiter) {
        return false;
    }
    deviceCode = line.substr(8);
    return !deviceCode.empty();
}

bool decodeOnline(const std::string& line, std::string& deviceCode)
{
    if (line.size() < 8 || line.compare(0, 6, kOnlinePrefix) != 0 || line[6] != kDelimiter) {
        return false;
    }
    deviceCode = line.substr(7);
    return !deviceCode.empty();
}

// ========== 主站 ↔ 遥控协议（CTRL 帧） ==========

namespace {

const char kCtrlPrefix[] = "CTRL";
const char kCtrlAckPrefix[] = "CTRL_ACK";

}  // namespace

std::string encodeCtrl(const std::string& deviceCode, int switchVal)
{
    std::ostringstream oss;
    oss << kCtrlPrefix << kDelimiter << deviceCode << kDelimiter << switchVal;
    return oss.str();
}

bool decodeCtrl(const std::string& line, std::string& deviceCode, int& switchVal)
{
    if (line.size() < 10 || line.compare(0, 4, kCtrlPrefix) != 0 || line[4] != kDelimiter) {
        return false;
    }
    /* 跳过 CTRL| */
    std::istringstream iss(line.substr(5));
    std::string token;
    if (!std::getline(iss, deviceCode, kDelimiter) || deviceCode.empty()) return false;
    if (!std::getline(iss, token)) return false;
    switchVal = std::atoi(token.c_str());
    return true;
}

std::string encodeCtrlAck(const std::string& deviceCode, int switchVal, bool success)
{
    std::ostringstream oss;
    oss << kCtrlAckPrefix << kDelimiter << deviceCode << kDelimiter
        << switchVal << kDelimiter << (success ? "SUCCESS" : "FAILURE");
    return oss.str();
}

}  // namespace protocol
}  // namespace scada
