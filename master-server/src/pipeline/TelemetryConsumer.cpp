/**
 * @file   TelemetryConsumer.cpp
 * @brief  遥测消费者实现（迭代3：控制台打印 + 推 Qt）
 * @module master-server
 */

#include "pipeline/TelemetryConsumer.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>

#include "push/UiBroadcaster.hpp"

namespace master {
namespace pipeline {

TelemetryConsumer::TelemetryConsumer()
    : broadcaster_(NULL)
{
}

void TelemetryConsumer::setBroadcaster(push::UiBroadcaster* broadcaster)
{
    broadcaster_ = broadcaster;
}

void TelemetryConsumer::onTelemetry(const scada::Telemetry& telemetry)
{
    /* 控制台输出（迭代 2 已有，保留用于调试） */
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

    /* 推送至 Qt 客户端（通过 UiBroadcaster，端口 5002） */
    if (broadcaster_ != NULL) {
        broadcaster_->broadcast(telemetry);
    }
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
