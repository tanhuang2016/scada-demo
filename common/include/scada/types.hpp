#pragma once

#include <cstdint>
#include <string>

namespace scada {

using DeviceId = std::string;

enum class SwitchState : std::uint8_t {
    Open = 0,
    Closed = 1,
};

struct Telemetry {
    DeviceId deviceId;
    double voltage = 0.0;
    double current = 0.0;
    SwitchState switchState = SwitchState::Open;
    std::int64_t timestamp = 0;
    bool alarm = false;
};

}  // namespace scada
