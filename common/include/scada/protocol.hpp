#pragma once

#include <optional>
#include <string>

#include "scada/types.hpp"

namespace scada::protocol {

std::string encodeTelemetry(const Telemetry& telemetry);
std::optional<Telemetry> decodeTelemetry(const std::string& line);

bool isVoltageAlarm(double voltage);

}  // namespace scada::protocol
