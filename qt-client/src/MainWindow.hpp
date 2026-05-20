#pragma once

/**
 * @file   MainWindow.hpp
 * @brief  Qt 主窗口——持有实时监控页和主站 TCP 客户端
 * @module qt-client
 */

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MasterClient;
class MonitorPage;

/**
 * @brief SCADA Qt 客户端主窗口
 *
 * 构造时：
 *   1. 创建 MasterClient → 连接主站 5002 端口（自动重连）
 *   2. 创建 MonitorPage → 添加到 pageContainer 容器
 *   3. 连接 MasterClient 信号到 MonitorPage 槽
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow* ui;
    MasterClient* client_;
    MonitorPage* monitorPage_;
};
