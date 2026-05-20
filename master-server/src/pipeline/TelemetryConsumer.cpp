/**
 * @file   TelemetryConsumer.cpp
 * @brief  遥测消费者实现（迭代7：+告警引擎判定）
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

void TelemetryConsumer::onTelemetry(scada::Telemetry& telemetry)
{
    /*
     * 告警判定（必须在 broadcast 之前，这样 alarm 字段可被编码进 UPDATE 帧）
     * AlarmEngine 内部做去重和 MySQL 持久化。
     */
    alarmEngine_.checkTelemetry(telemetry);

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
              << (telemetry.alarm ? " [告警]" : "")
              << std::endl;

    if (broadcaster_ == NULL) return;

    const std::string& deviceCode = telemetry.deviceId;
    std::map<std::string, bool>::iterator it = onlineMap_.find(deviceCode);
    bool wasOnline = (it != onlineMap_.end() && it->second);

    if (!wasOnline) {
        onlineMap_[deviceCode] = true;
        std::string onlineFrame = scada::protocol::encodeOnline(deviceCode);
        broadcaster_->sendLine(onlineFrame);
        alarmEngine_.onDeviceOnline(deviceCode);
        std::cout << "[状态] " << deviceCode << " 上线\n";
    }

    /* 写入 MySQL 历史库 */
    recorder_.record(telemetry);

    /* 推送 UPDATE 帧到 Qt（alarm 字段已设置） */
    broadcaster_->broadcast(telemetry);
}

void TelemetryConsumer::onDeviceOffline(const std::string& deviceCode)
{
    std::cout << "[离线] " << deviceCode << " 连接已断开\n";

    /* 通信中断告警 */
    alarmEngine_.onDeviceOffline(deviceCode);

    if (broadcaster_ == NULL) return;

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
