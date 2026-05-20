/**
 * @file   MainWindow.cpp
 * @brief  主窗口：创建 3 卡片 + MasterClient，连接信号槽
 * @module qt-client
 */

#include "MainWindow.hpp"

#include "ui_MainWindow.h"

#include <QVBoxLayout>

#include "net/MasterClient.hpp"
#include "pages/MonitorPage.hpp"
#include "scada/config_defaults.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , client_(NULL)
{
    ui->setupUi(this);

    /* 创建 3 张设备监控卡片，添加到 cardsLayout（水平布局） */
    const char* deviceCodes[3] = { "RTU001", "RTU002", "RTU003" };

    for (int i = 0; i < 3; ++i) {
        cards_[i] = new MonitorPage(this);
        cards_[i]->setDeviceCode(QString::fromUtf8(deviceCodes[i]));
        ui->cardsLayout->addWidget(cards_[i]);
    }

    /* 创建主站 TCP 客户端 */
    client_ = new MasterClient(this);

    /*
     * 信号槽连接（旧式 SIGNAL/SLOT 宏，兼容自定义类型 scada::Telemetry）：
     *   telemetryReceived → 所有卡片（MonitorPage 内部按 deviceId 过滤）
     *   connectionStateChanged → 所有卡片（主站→Qt 链路）
     *   deviceOnline / deviceOffline → 所有卡片（单设备在线/离线，按 deviceId 过滤）
     */
    for (int i = 0; i < 3; ++i) {
        QObject::connect(client_, SIGNAL(telemetryReceived(scada::Telemetry)),
                         cards_[i], SLOT(onTelemetry(scada::Telemetry)));
        QObject::connect(client_, SIGNAL(connectionStateChanged(bool)),
                         cards_[i], SLOT(onConnectionStateChanged(bool)));
        QObject::connect(client_, SIGNAL(deviceOnline(QString)),
                         cards_[i], SLOT(onDeviceOnline(QString)));
        QObject::connect(client_, SIGNAL(deviceOffline(QString)),
                         cards_[i], SLOT(onDeviceOffline(QString)));
    }

    /* 连接主站 5002 推送端口 */
    client_->connectToMaster(QString::fromUtf8("127.0.0.1"),
                             scada::config::kMasterToUiPort);

    statusBar()->showMessage(tr("就绪 — 监控 3 台设备"));
}

MainWindow::~MainWindow()
{
    delete ui;
}
