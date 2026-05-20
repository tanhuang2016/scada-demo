/**
 * @file   TelemetryConsumer.cpp
 * @brief  遥测消费者实现（迭代2：控制台打印）
 * @module master-server
 */

#include "pipeline/TelemetryConsumer.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace master {
namespace pipeline {

void TelemetryConsumer::onTelemetry(const scada::Telemetry& telemetry)
{
    /* 格式化时间 */
    std::time_t ts = static_cast<std::time_t>(telemetry.timestamp);
    char timeBuf[32] = {};
#ifdef _WIN32
    struct tm tmBuf;
    localtime_s(&tmBuf, &ts);
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmBuf);
#else
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", std::localtime(&ts));
#endif

    const char* switchStr = (telemetry.switchState == scada::SwitchState::Closed) ? "合闸" : "分闸";

    std::cout << "[遥测] " << telemetry.deviceId
              << " | 电压: " << std::fixed << std::setprecision(1) << telemetry.voltage << "V"
              << " | 电流: " << std::setprecision(2) << telemetry.current << "A"
              << " | 开关: " << switchStr
              << " | 时间: " << timeBuf
              << std::endl;
}

void TelemetryConsumer::onDeviceOffline(const std::string& deviceCode)
{
    std::cout << "[离线] " << deviceCode << " 连接已断开\n";
}

void TelemetryConsumer::reportNotImplemented(const std::string& deviceCode, const char* protocolName)
{
    std::cout << "[跳过] " << deviceCode << " 协议 " << protocolName << " 未实现\n";
}

}  // namespace pipeline
}  // namespace master
