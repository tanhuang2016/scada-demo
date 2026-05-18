#pragma once

/**
 * @file   types.hpp
 * @brief  设备遥测、开关等领域类型（与 MySQL 业务模型对齐）
 */

#include <cstdint>
#include <string>

namespace scada {

/** 设备唯一编码，如 RTU001 */
typedef std::string DeviceId;

/** 开关位置：分闸 / 合闸 */
enum class SwitchState {
    Open = 0,
    Closed = 1,
};

/**
 * @brief 单台设备一次遥测快照（内存态，可由 104 或文本协议填充）
 */
struct Telemetry {
    DeviceId deviceId;
    double voltage;
    double current;
    SwitchState switchState;
    std::int64_t timestamp;
    bool alarm;

    Telemetry()
        : voltage(0.0)
        , current(0.0)
        , switchState(SwitchState::Open)
        , timestamp(0)
        , alarm(false)
    {
    }
};

}  // namespace scada
