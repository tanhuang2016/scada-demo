/**
 * @file   MonitorPage.cpp
 * @brief  实时监控页实现——设备过滤 + 遥测卡片更新 + 在线/离线状态色
 * @module qt-client
 */

#include "pages/MonitorPage.hpp"
#include "ui_MonitorPage.h"

#include <iomanip>
#include <sstream>

#include "scada/types.hpp"

MonitorPage::MonitorPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::MonitorPage)
    , deviceCode_()
    , lastUpdateTime_(0)
    , deviceOnline_(false)
    , masterOnline_(false)
{
    qRegisterMetaType<scada::Telemetry>("scada::Telemetry");
    ui->setupUi(this);
}

MonitorPage::~MonitorPage()
{
    delete ui;
}

void MonitorPage::setDeviceCode(const QString& code)
{
    deviceCode_ = code;
    ui->deviceNameLabel->setText(code);
}

/*
 * 收到遥测数据——仅当 deviceId 匹配本卡片才更新。
 *
 * 字段格式化：
 *   - 电压：保留 1 位小数 + " V"
 *   - 电流：保留 2 位小数 + " A"
 *   - 开关：Closed→"合闸"(绿色)，Open→"分闸"(红色)
 *   - 时间：HH:MM:SS
 */
void MonitorPage::onTelemetry(const scada::Telemetry& telemetry)
{
    QString id = QString::fromStdString(telemetry.deviceId);
    if (id != deviceCode_) return;  // 不属于本卡片

    /*
     * 隐式上线：能收到遥测数据说明设备一定在线。
     *
     * 为什么不用依赖 ONLINE 帧：
     *   Qt 客户端可能在设备已运行数分钟后才启动。
     *   此时 ONLINE 帧早已发送（当时无客户端监听），
     *   之后只能收到 UPDATE 帧。因此收到数据本身即"上线"信号。
     *   OFFLINE 帧仍然由主站主动推送（设备断线时），
     *   确保离线状态能被及时感知。
     */
    if (!deviceOnline_) {
        deviceOnline_ = true;
        if (masterOnline_) {
            setOnline(true);
        }
    }

    /* 电压 */
    std::ostringstream voss;
    voss << std::fixed << std::setprecision(1) << telemetry.voltage << " V";
    ui->voltageValue->setText(QString::fromStdString(voss.str()));

    /* 电流 */
    std::ostringstream coss;
    coss << std::fixed << std::setprecision(2) << telemetry.current << " A";
    ui->currentValue->setText(QString::fromStdString(coss.str()));

    /* 开关状态 */
    const char* switchText = (telemetry.switchState == scada::SwitchState::Closed)
                           ? "合闸" : "分闸";
    ui->switchValue->setText(QString::fromUtf8(switchText));

    /* 最后更新时间 */
    lastUpdateTime_ = static_cast<std::time_t>(telemetry.timestamp);
    char timeBuf[16] = {};
#ifdef _WIN32
    struct tm tmBuf;
    localtime_s(&tmBuf, &lastUpdateTime_);
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmBuf);
#else
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S",
                  std::localtime(&lastUpdateTime_));
#endif
    ui->updateTimeLabel->setText(
        QString::fromUtf8("最后更新: ") + QString::fromUtf8(timeBuf));
}

/*
 * 主站→Qt 推送链路状态变化。
 *
 * 绿色"在线"仅在两项条件同时满足时显示：
 *   1. masterOnline_ = true（主站→Qt 链路连通）
 *   2. deviceOnline_ = true（该设备→主站链路在线）
 * 任一不满足则显示"等待连接..."（灰色）。
 */
void MonitorPage::onConnectionStateChanged(bool connected)
{
    masterOnline_ = connected;
    if (masterOnline_) {
        /* 主站链路恢复：卡片颜色取决于设备是否在线 */
        setOnline(deviceOnline_);
    } else {
        setOnline(false);
    }
}

void MonitorPage::onDeviceOnline(const QString& deviceId)
{
    if (deviceId != deviceCode_) return;
    deviceOnline_ = true;
    setOnline(true);
}

void MonitorPage::onDeviceOffline(const QString& deviceId)
{
    if (deviceId != deviceCode_) return;
    deviceOnline_ = false;
    setOnline(false);
}

void MonitorPage::setOnline(bool online)
{
    if (online) {
        ui->statusLabel->setText(QString::fromUtf8("在线"));
        ui->statusLabel->setStyleSheet("color: green; font-weight: bold;");
    } else {
        ui->statusLabel->setText(QString::fromUtf8("等待连接..."));
        ui->statusLabel->setStyleSheet("color: gray;");
    }
}
