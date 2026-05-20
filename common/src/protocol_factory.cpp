/**
 * @file   protocol_factory.cpp
 * @brief  设备协议工厂
 */

#include "scada/protocol_factory.hpp"

#include <algorithm>
#include <cctype>

#include "scada/iec104_protocol.hpp"
#include "scada/json_protocol.hpp"

namespace scada {
namespace device_protocol {

namespace {

std::string toUpper(std::string s)
{
    for (std::size_t i = 0; i < s.size(); ++i) {
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
    }
    return s;
}

}  // namespace

std::unique_ptr<IDeviceProtocol> createProtocol(ProtocolKind kind)
{
    if (kind == kIec104) {
        return std::unique_ptr<IDeviceProtocol>(new Iec104Protocol());
    }
    return std::unique_ptr<IDeviceProtocol>(new JsonProtocol());
}

std::unique_ptr<IDeviceProtocol> createProtocolFromName(const std::string& protocolName,
                                                        ProtocolKind& outKind)
{
    const std::string upper = toUpper(protocolName);
    if (upper == "IEC104" || upper == "IEC_104" || upper == "104") {
        outKind = kIec104;
    } else {
        outKind = kJson;
    }
    return createProtocol(outKind);
}

}  // namespace device_protocol
}  // namespace scada
