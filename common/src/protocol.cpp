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
 * 格式：UPDATE|deviceId|voltage|current|switch|timestamp
 * switch: static_cast<int>(switchState) → Closed=1, Open=0
 */
std::string encodeUpdate(const Telemetry& telem)
{
    std::ostringstream oss;
    oss << kUpdatePrefix << kDelimiter << telem.deviceId << kDelimiter
        << telem.voltage << kDelimiter << telem.current << kDelimiter
        << static_cast<int>(telem.switchState) << kDelimiter << telem.timestamp;
    return oss.str();
}

/**
 * 解析 UPDATE 帧。
 * 格式固定为 6 个字段，分隔符为 '|'。
 */
bool decodeUpdate(const std::string& line, Telemetry& out)
{
    /* 前缀必须为 UPDATE| */
    if (line.size() < 7 || line.compare(0, 6, kUpdatePrefix) != 0 || line[6] != kDelimiter) {
        return false;
    }

    Telemetry telemetry;
    std::istringstream iss(line);
    std::string token;
    int switchVal = 0;

    /* 跳过前缀 */
    if (!std::getline(iss, token, kDelimiter)) return false;  // UPDATE

    /* deviceId */
    if (!std::getline(iss, telemetry.deviceId, kDelimiter) || telemetry.deviceId.empty()) return false;

    /* voltage */
    if (!std::getline(iss, token, kDelimiter)) return false;
    telemetry.voltage = std::atof(token.c_str());

    /* current */
    if (!std::getline(iss, token, kDelimiter)) return false;
    telemetry.current = std::atof(token.c_str());

    /* switch */
    if (!std::getline(iss, token, kDelimiter)) return false;
    switchVal = std::atoi(token.c_str());

    /* timestamp (最后一个字段，用 getline 读到换行或结束) */
    if (!std::getline(iss, token)) return false;
    telemetry.timestamp = static_cast<std::int64_t>(std::atoll(token.c_str()));

    telemetry.switchState = (switchVal != 0) ? SwitchState::Closed : SwitchState::Open;
    telemetry.alarm = isVoltageAlarm(telemetry.voltage);
    out = telemetry;
    return true;
}

}  // namespace protocol
}  // namespace scada
