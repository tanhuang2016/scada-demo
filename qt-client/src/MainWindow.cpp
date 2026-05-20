/**
 * @file   MainWindow.cpp
 * @brief  主窗口：创建 MasterClient + MonitorPage，连接信号槽
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
    , monitorPage_(NULL)
{
    ui->setupUi(this);

    /* 创建监控页面并添加到容器 */
    monitorPage_ = new MonitorPage(this);
    QLayout* pageLayout = ui->pageContainer->layout();
    if (pageLayout != NULL) {
        pageLayout->addWidget(monitorPage_);
    }

    /* 创建主站 TCP 客户端 */
    client_ = new MasterClient(this);
    QObject::connect(client_, SIGNAL(telemetryReceived(scada::Telemetry)),
                     monitorPage_, SLOT(onTelemetry(scada::Telemetry)));
    QObject::connect(client_, SIGNAL(connectionStateChanged(bool)),
                     monitorPage_, SLOT(onConnectionStateChanged(bool)));

    /* 连接主站 5002 推送端口 */
    client_->connectToMaster(QString::fromUtf8("127.0.0.1"),
                             scada::config::kMasterToUiPort);

    statusBar()->showMessage(tr("就绪"));
}

MainWindow::~MainWindow()
{
    /*
     * Qt 父子关系自动管理对象生命周期：
     * client_ 和 monitorPage_ 的 parent 都是 this，
     * QMainWindow 析构时会自动 delete。
     * ui 显式 delete 即可。
     */
    delete ui;
}
