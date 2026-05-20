#pragma once

/**
 * @file   iec104_protocol.hpp
 * @brief  IEC 60870-5-104 协议预留实现（当前未启用）
 *
 * 后续若补充 104，在本类内实现 IDeviceProtocol，业务层无需改动。
 */

#include "scada/device_protocol.hpp"

namespace scada {
namespace device_protocol {

/**
 * @brief IEC104 占位：isImplemented() 为 false，编解码均返回 false
 */
class Iec104Protocol : public IDeviceProtocol {
public:
    virtual ProtocolKind kind() const;
    virtual const char* name() const;
    virtual bool isImplemented() const;

    virtual bool encodeTelemetry(const Telemetry& in, std::string& out) const;
    virtual bool decodeTelemetry(const std::string& frame, Telemetry& out) const;
    virtual bool encodeControl(const DeviceId& deviceId, SwitchState state, std::string& out) const;
};

}  // namespace device_protocol
}  // namespace scada
