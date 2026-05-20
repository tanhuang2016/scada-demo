#pragma once

/**
 * @file   protocol_factory.hpp
 * @brief  按配置创建 JSON / IEC104 协议实例
 */

#include <memory>
#include <string>

#include "scada/device_protocol.hpp"

namespace scada {
namespace device_protocol {

/**
 * @brief 根据枚举创建协议对象
 * @param kind  kJson 或 kIec104
 */
std::unique_ptr<IDeviceProtocol> createProtocol(ProtocolKind kind);

/**
 * @brief 根据 device 表 protocol 字段创建（"JSON" / "IEC104"，大小写不敏感）
 * @param protocolName  配置库中的字符串
 * @param outKind       解析出的种类
 */
std::unique_ptr<IDeviceProtocol> createProtocolFromName(const std::string& protocolName,
                                                        ProtocolKind& outKind);

}  // namespace device_protocol
}  // namespace scada
