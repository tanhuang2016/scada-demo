/**
 * @file   MainWindow.cpp
 * @brief  主窗口：标签页（监控 + 设备管理）+ MySQL + 信号槽
 * @module qt-client
 */

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include <QVBoxLayout>

#include "net/MasterClient.hpp"
#include "pages/DeviceManagePage.hpp"
#include "pages/MonitorPage.hpp"
#include "storage/DeviceManager.hpp"
#include "scada/config_defaults.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , deviceMgr_(NULL)
    , client_(NULL)
    , managePage_(NULL)
{
    ui->setupUi(this);

    /* 初始化 MySQL 连接 */
    deviceMgr_ = new DeviceManager();
    if (!deviceMgr_->initialize("127.0.0.1", 3306, "root", "tanhuang", "scada_demo")) {
        statusBar()->showMessage(tr("MySQL 连接失败 — 设备管理不可用"));
    } else {
        statusBar()->showMessage(tr("MySQL 已连接"));
    }

    /* Tab 1：实时监控 — 3 张设备卡片 */
    const char* deviceCodes[3] = { "RTU001", "RTU002", "RTU003" };
    for (int i = 0; i < 3; ++i) {
        cards_[i] = new MonitorPage(this);
        cards_[i]->setDeviceCode(QString::fromUtf8(deviceCodes[i]));
        ui->cardsLayout->addWidget(cards_[i]);
    }

    /* Tab 2：设备管理 */
    managePage_ = new DeviceManagePage(deviceMgr_, this);
    ui->manageLayout->addWidget(managePage_);

    /* 主站 TCP 客户端 */
    client_ = new MasterClient(this);

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

    client_->connectToMaster(QString::fromUtf8("127.0.0.1"),
                             scada::config::kMasterToUiPort);
}

MainWindow::~MainWindow()
{
    delete ui;
}
