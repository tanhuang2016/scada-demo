/**
 * @file   json_protocol.cpp
 * @brief  设备侧 JSON 行协议（C++11，轻量字符串拼包/解析）
 */

#include "scada/json_protocol.hpp"

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace scada {
namespace device_protocol {

namespace {

/** 从 json 片段读取 "key":number */
bool extractNumber(const std::string& json, const char* key, double& value)
{
    const std::string pattern = std::string("\"") + key + "\":";
    const std::size_t pos = json.find(pattern);
    if (pos == std::string::npos) {
        return false;
    }
    value = std::atof(json.c_str() + pos + pattern.size());
    return true;
}

bool extractInt(const std::string& json, const char* key, int& value)
{
    double n = 0.0;
    if (!extractNumber(json, key, n)) {
        return false;
    }
    value = static_cast<int>(n);
    return true;
}

bool extractString(const std::string& json, const char* key, std::string& value)
{
    const std::string pattern = std::string("\"") + key + "\":\"";
    const std::size_t start = json.find(pattern);
    if (start == std::string::npos) {
        return false;
    }
    const std::size_t from = start + pattern.size();
    const std::size_t end = json.find('"', from);
    if (end == std::string::npos) {
        return false;
    }
    value = json.substr(from, end - from);
    return true;
}

}  // namespace

ProtocolKind JsonProtocol::kind() const
{
    return kJson;
}

const char* JsonProtocol::name() const
{
    return "JSON";
}

bool JsonProtocol::isImplemented() const
{
    return true;
}

bool JsonProtocol::encodeTelemetry(const Telemetry& in, std::string& out) const
{
    std::ostringstream oss;
    oss << "{\"type\":\"telemetry\",\"device\":\"" << in.deviceId << "\",\"voltage\":" << in.voltage
        << ",\"current\":" << in.current << ",\"switch\":" << static_cast<int>(in.switchState)
        << ",\"ts\":" << in.timestamp << "}\n";
    out = oss.str();
    return true;
}

bool JsonProtocol::decodeTelemetry(const std::string& frame, Telemetry& out) const
{
    if (frame.find("\"type\":\"telemetry\"") == std::string::npos &&
        frame.find("\"type\": \"telemetry\"") == std::string::npos) {
        return false;
    }

    Telemetry t;
    if (!extractString(frame, "device", t.deviceId) || t.deviceId.empty()) {
        return false;
    }
    if (!extractNumber(frame, "voltage", t.voltage)) {
        return false;
    }
    if (!extractNumber(frame, "current", t.current)) {
        return false;
    }
    int sw = 0;
    if (!extractInt(frame, "switch", sw)) {
        return false;
    }
    t.switchState = (sw != 0) ? SwitchState::Closed : SwitchState::Open;

    int ts = 0;
    if (extractInt(frame, "ts", ts)) {
        t.timestamp = ts;
    }

    out = t;
    return true;
}

bool JsonProtocol::encodeControl(const DeviceId& deviceId, SwitchState state, std::string& out) const
{
    std::ostringstream oss;
    oss << "{\"type\":\"control\",\"device\":\"" << deviceId << "\",\"switch\":"
        << static_cast<int>(state) << "}\n";
    out = oss.str();
    return true;
}

}  // namespace device_protocol
}  // namespace scada
