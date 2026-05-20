#pragma once

/**
 * @file   MonitorPage.hpp
 * @brief  实时监控页——展示单设备遥测卡片（电压/电流/开关/时间/状态）
 * @module qt-client
 */

#include <QWidget>
#include <QString>
#include <ctime>

namespace Ui {
class MonitorPage;
}

namespace scada {
struct Telemetry;
}  // namespace scada

/**
 * @brief 实时监控页 Widget，对应 ui/MonitorPage.ui
 *
 * 每张卡片绑定一台设备（通过 setDeviceCode 设置）。
 * 同时监听 MasterClient 的多个信号：
 *   - telemetryReceived → 仅 deviceId 匹配时更新数据
 *   - connectionStateChanged → master→Qt 链路状态（整体在线/离线）
 *   - deviceOnline / deviceOffline → 单设备在线/离线（设备→主站链路）
 */
class MonitorPage : public QWidget {
    Q_OBJECT

public:
    explicit MonitorPage(QWidget* parent = 0);
    ~MonitorPage();

    /** @brief 设置本卡片监听的设备编码，同时更新标题 */
    void setDeviceCode(const QString& code);

public slots:
    void onTelemetry(const scada::Telemetry& telemetry);
    void onConnectionStateChanged(bool connected);
    void onDeviceOnline(const QString& deviceId);
    void onDeviceOffline(const QString& deviceId);
    void onCloseSwitch();
    void onOpenSwitch();

private:
    void setOnline(bool online);

    /** @brief 发送遥控命令到主站端口 5003 */
    void sendControl(int switchVal);

    Ui::MonitorPage* ui;
    QString deviceCode_;
    std::time_t lastUpdateTime_;
    bool deviceOnline_;    ///< 该设备是否在线（设备→主站链路）
    bool masterOnline_;    ///< 主站→Qt 推送链路是否连通
};
