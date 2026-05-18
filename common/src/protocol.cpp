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

}  // namespace protocol
}  // namespace scada
