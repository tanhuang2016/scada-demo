/**
 * @file   iec104_protocol.cpp
 * @brief  IEC104 预留：未实现，便于后续在同一接口下接入 lib60870 等
 */

#include "scada/iec104_protocol.hpp"

namespace scada {
namespace device_protocol {

ProtocolKind Iec104Protocol::kind() const
{
    return kIec104;
}

const char* Iec104Protocol::name() const
{
    return "IEC104";
}

bool Iec104Protocol::isImplemented() const
{
    return false;
}

bool Iec104Protocol::encodeTelemetry(const Telemetry&, std::string&) const
{
    return false;
}

bool Iec104Protocol::decodeTelemetry(const std::string&, Telemetry&) const
{
    return false;
}

bool Iec104Protocol::encodeControl(const DeviceId&, SwitchState, std::string&) const
{
    return false;
}

}  // namespace device_protocol
}  // namespace scada
