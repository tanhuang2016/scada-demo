#pragma once

/**
 * @file   MainWindow.hpp
 * @brief  Qt 主窗口——3 列设备监控卡片 + 主站 TCP 客户端
 * @module qt-client
 */

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MasterClient;
class MonitorPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow* ui;
    MasterClient* client_;
    MonitorPage* cards_[3];
};
