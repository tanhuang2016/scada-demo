#pragma once

/**
 * @file   MainWindow.hpp
 * @brief  Qt 主窗口——标签页（监控 + 设备管理）+ 主站 TCP 客户端
 * @module qt-client
 */

#include <QMainWindow>

namespace Ui { class MainWindow; }

class DeviceManager;
class DeviceManagePage;
class MasterClient;
class MonitorPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow* ui;
    DeviceManager* deviceMgr_;
    MasterClient* client_;
    MonitorPage* cards_[3];
    DeviceManagePage* managePage_;
};
