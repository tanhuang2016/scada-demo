#pragma once

/**
 * @file   json_protocol.hpp
 * @brief  设备侧 JSON 行协议（当前默认实现）
 */

#include "scada/device_protocol.hpp"

namespace scada {
namespace device_protocol {

/**
 * @brief JSON 单行协议，详见 docs/protocol-device-json.md
 */
class JsonProtocol : public IDeviceProtocol {
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
