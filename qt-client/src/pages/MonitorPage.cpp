/**
 * @file   MonitorPage.cpp
 * @brief  实时监控页实现——接收遥测信号并更新卡片显示
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
    , lastUpdateTime_(0)
{
    qRegisterMetaType<scada::Telemetry>("scada::Telemetry");

    ui->setupUi(this);
}

MonitorPage::~MonitorPage()
{
    delete ui;
}

/*
 * 收到遥测数据，更新卡片各字段。
 *
 * 字段格式化：
 *   - 电压：保留 1 位小数 + "V"
 *   - 电流：保留 2 位小数 + "A"
 *   - 开关：Closed→"合闸"，Open→"分闸"
 *   - 时间：HH:MM:SS
 */
void MonitorPage::onTelemetry(const scada::Telemetry& telemetry)
{
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
 * 连接状态变化回调。
 *
 * 在线：状态标签文字变绿，显示"在线"
 * 离线：状态标签文字变灰，显示"等待连接..."
 */
void MonitorPage::onConnectionStateChanged(bool connected)
{
    if (connected) {
        ui->statusLabel->setText(QString::fromUtf8("在线"));
        ui->statusLabel->setStyleSheet("color: green; font-weight: bold;");
    } else {
        ui->statusLabel->setText(QString::fromUtf8("等待连接..."));
        ui->statusLabel->setStyleSheet("color: gray;");
        /* 离线时保留最后一次遥测值不清除（符合 SCADA 惯例） */
    }
}
