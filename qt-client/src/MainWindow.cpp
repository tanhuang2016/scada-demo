/**
 * @file   MainWindow.cpp
 * @brief  主窗口：setupUi 加载 .ui，代码中仅更新动态文案与状态栏
 */

#include "MainWindow.hpp"

#include "ui_MainWindow.h"

#include "scada/config_defaults.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* 占位文案：迭代 3 连接主站后改为在线设备数等 */
    ui->statusLabel->setText(
        tr("脚手架模式 — 主站推送端口 %1（详见迭代 3）").arg(scada::config::kMasterToUiPort));
    statusBar()->showMessage(tr("就绪"));
}

MainWindow::~MainWindow()
{
    delete ui;
}
