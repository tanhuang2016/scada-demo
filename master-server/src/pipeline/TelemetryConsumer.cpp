/**
 * @file   TelemetryConsumer.cpp
 * @brief  遥测消费者实现（迭代4：多设备 + 在线状态跟踪）
 * @module master-server
 */

#include "pipeline/TelemetryConsumer.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>

#include "push/UiBroadcaster.hpp"
#include "scada/protocol.hpp"

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
    /* 控制台输出 */
    std::time_t ts = static_cast<std::time_t>(telemetry.timestamp);
    char timeBuf[32] = {};
#ifdef _WIN32
    struct tm tmBuf;
    localtime_s(&tmBuf, &ts);
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmBuf);
#else
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", std::localtime(&ts));
#endif

    const char* switchStr = (telemetry.switchState == scada::SwitchState::Closed)
                          ? "合闸" : "分闸";

    std::cout << "[遥测] " << telemetry.deviceId
              << " | 电压: " << std::fixed << std::setprecision(1) << telemetry.voltage << "V"
              << " | 电流: " << std::setprecision(2) << telemetry.current << "A"
              << " | 开关: " << switchStr
              << " | 时间: " << timeBuf
              << std::endl;

    if (broadcaster_ == NULL) return;

    /*
     * 跟踪在线状态：如果之前为离线或未记录，发送 ONLINE 通知到 Qt。
     * 这样 Qt 端可以将卡片从灰色切换为绿色。
     */
    const std::string& deviceCode = telemetry.deviceId;
    std::map<std::string, bool>::iterator it = onlineMap_.find(deviceCode);
    bool wasOnline = (it != onlineMap_.end() && it->second);

    if (!wasOnline) {
        onlineMap_[deviceCode] = true;
        std::string onlineFrame = scada::protocol::encodeOnline(deviceCode);
        broadcaster_->sendLine(onlineFrame);
        std::cout << "[状态] " << deviceCode << " 上线\n";
    }

    /* 推送 UPDATE 帧到 Qt */
    broadcaster_->broadcast(telemetry);
}

void TelemetryConsumer::onDeviceOffline(const std::string& deviceCode)
{
    std::cout << "[离线] " << deviceCode << " 连接已断开\n";

    if (broadcaster_ == NULL) return;

    /*
     * 去重：如果已是离线状态则不重复发送 OFFLINE 帧。
     */
    std::map<std::string, bool>::iterator it = onlineMap_.find(deviceCode);
    bool wasOnline = (it != onlineMap_.end() && it->second);

    if (wasOnline || it == onlineMap_.end()) {
        onlineMap_[deviceCode] = false;
        std::string offlineFrame = scada::protocol::encodeOffline(deviceCode);
        broadcaster_->sendLine(offlineFrame);
        std::cout << "[状态] " << deviceCode << " 离线通知已发送\n";
    }
}

void TelemetryConsumer::reportNotImplemented(const std::string& deviceCode,
                                              const char* protocolName)
{
    std::cout << "[跳过] " << deviceCode << " 协议 " << protocolName << " 未实现\n";
}

}  // namespace pipeline
}  // namespace master
