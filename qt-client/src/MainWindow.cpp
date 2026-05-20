/**
 * @file   MainWindow.cpp
 * @brief  主窗口：动态监控卡片 + 设备管理 + MySQL
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

    /* 主站 TCP 客户端（先创建，卡片创建时连接信号） */
    client_ = new MasterClient(this);

    /*
     * Tab 1：实时监控 — 从 MySQL 动态加载设备列表创建卡片
     *
     * 卡片数量由 MySQL device 表中 enabled=1 的记录数决定。
     * 设备增删后调用 refreshMonitorCards() 重建卡片。
     */
    refreshMonitorCards();

    /* Tab 2：设备管理 */
    managePage_ = new DeviceManagePage(deviceMgr_, this);
    ui->manageLayout->addWidget(managePage_);

    /* 设备增删改后自动刷新监控卡片 */
    QObject::connect(managePage_, SIGNAL(devicesChanged()),
                     this, SLOT(refreshMonitorCards()));

    client_->connectToMaster(QString::fromUtf8("127.0.0.1"),
                             scada::config::kMasterToUiPort);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*
 * 从 MySQL 加载启用设备列表，动态创建监控卡片。
 *
 * 先清除所有旧卡片，再按设备列表重建。
 * 每张卡片连接 MasterClient 的遥测/在线/离线信号。
 */
void MainWindow::refreshMonitorCards()
{
    /* 清除旧卡片 */
    for (QVector<MonitorPage*>::iterator it = cards_.begin();
         it != cards_.end(); ++it) {
        ui->cardsLayout->removeWidget(*it);
        delete *it;
    }
    cards_.clear();

    if (deviceMgr_ == NULL) return;

    /* 从 MySQL 加载设备列表 */
    std::vector<DeviceInfo> devices;
    deviceMgr_->loadAllDevices(devices);

    for (std::vector<DeviceInfo>::iterator it = devices.begin();
         it != devices.end(); ++it) {
        if (!it->enabled) continue;  // 只显示启用的设备

        MonitorPage* card = new MonitorPage(this);
        card->setDeviceCode(QString::fromStdString(it->deviceCode));
        connectCard(card);
        ui->cardsLayout->addWidget(card);
        cards_.append(card);
    }
}

/*
 * 将一张卡片连接到 MasterClient 的四个信号。
 * 每个信号都发给所有卡片，卡片内部按 deviceCode 过滤。
 */
void MainWindow::connectCard(MonitorPage* card)
{
    QObject::connect(client_, SIGNAL(telemetryReceived(scada::Telemetry)),
                     card, SLOT(onTelemetry(scada::Telemetry)));
    QObject::connect(client_, SIGNAL(connectionStateChanged(bool)),
                     card, SLOT(onConnectionStateChanged(bool)));
    QObject::connect(client_, SIGNAL(deviceOnline(QString)),
                     card, SLOT(onDeviceOnline(QString)));
    QObject::connect(client_, SIGNAL(deviceOffline(QString)),
                     card, SLOT(onDeviceOffline(QString)));
}
