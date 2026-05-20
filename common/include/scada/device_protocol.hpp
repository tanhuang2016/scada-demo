#pragma once

/**
 * @file   device_protocol.hpp
 * @brief  设备侧协议抽象：业务层只依赖本接口，具体 JSON / IEC104 可替换
 */

#include <string>

#include "scada/types.hpp"

namespace scada {
namespace device_protocol {

/** 协议实现种类（与 device 表 protocol 字段对应） */
enum ProtocolKind {
    kJson = 0,
    kIec104 = 1,
};

/**
 * @brief 设备链路协议接口（模拟器 ↔ 主站）
 *
 * 业务层（采集、遥控路由）仅调用本接口，不感知 JSON 或 104 帧格式。
 */
class IDeviceProtocol {
public:
    virtual ~IDeviceProtocol() {}

    virtual ProtocolKind kind() const = 0;

    /** @brief 协议名称，如 "JSON" / "IEC104" */
    virtual const char* name() const = 0;

    /** @brief 当前实现是否可用（IEC104 预留类返回 false） */
    virtual bool isImplemented() const = 0;

    /**
     * @brief 编码遥测为上送帧（通常一行 JSON + 换行）
     */
    virtual bool encodeTelemetry(const Telemetry& in, std::string& out) const = 0;

    /**
     * @brief 从接收帧解析遥测
     */
    virtual bool decodeTelemetry(const std::string& frame, Telemetry& out) const = 0;

    /**
     * @brief 编码遥控命令帧
     */
    virtual bool encodeControl(const DeviceId& deviceId, SwitchState state, std::string& out) const = 0;
};

}  // namespace device_protocol
}  // namespace scada
