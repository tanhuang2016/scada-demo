#pragma once

namespace scada::config {

inline constexpr int kDeviceToMasterPort = 5001;
inline constexpr int kMasterToUiPort = 5002;
inline constexpr int kUiToMasterPort = 5003;

inline constexpr double kVoltageMin = 215.0;
inline constexpr double kVoltageMax = 235.0;

}  // namespace scada::config
