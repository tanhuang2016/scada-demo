#include "scada/protocol.hpp"

#include <sstream>

#include "scada/config_defaults.hpp"

namespace scada::protocol {

namespace {

constexpr char kDelimiter = '|';

}  // namespace

std::string encodeTelemetry(const Telemetry& telemetry)
{
    std::ostringstream oss;
    oss << "TELEM" << kDelimiter << telemetry.deviceId << kDelimiter << telemetry.voltage
        << kDelimiter << telemetry.current << kDelimiter
        << static_cast<int>(telemetry.switchState) << kDelimiter << telemetry.timestamp;
    return oss.str();
}

std::optional<Telemetry> decodeTelemetry(const std::string& line)
{
    if (line.rfind("TELEM", 0) != 0) {
        return std::nullopt;
    }

    Telemetry telemetry;
    char tag[16] = {};
    int switchState = 0;
    std::istringstream iss(line);
    char delimiter = 0;
    if (!(iss >> tag >> delimiter) || delimiter != kDelimiter) {
        return std::nullopt;
    }
    if (!(std::getline(iss, telemetry.deviceId, kDelimiter) && !telemetry.deviceId.empty())) {
        return std::nullopt;
    }
    if (!(iss >> telemetry.voltage >> delimiter && delimiter == kDelimiter)) {
        return std::nullopt;
    }
    if (!(iss >> telemetry.current >> delimiter && delimiter == kDelimiter)) {
        return std::nullopt;
    }
    if (!(iss >> switchState >> delimiter && delimiter == kDelimiter)) {
        return std::nullopt;
    }
    if (!(iss >> telemetry.timestamp)) {
        return std::nullopt;
    }

    telemetry.switchState = switchState != 0 ? SwitchState::Closed : SwitchState::Open;
    telemetry.alarm = isVoltageAlarm(telemetry.voltage);
    return telemetry;
}

bool isVoltageAlarm(double voltage)
{
    return voltage < config::kVoltageMin || voltage > config::kVoltageMax;
}

}  // namespace scada::protocol
