#pragma once

/**
 * @file   MainWindow.hpp
 * @brief  Qt 主窗口——标签页（监控 + 设备管理）+ 动态卡片
 * @module qt-client
 */

#include <QMainWindow>
#include <QVector>

namespace Ui { class MainWindow; }
class DeviceManager;
class DeviceManagePage;
class MasterClient;
class MonitorPage;
class OperationLogPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

public slots:
    /** @brief 从 MySQL 重新加载设备列表，刷新监控卡片（设备增删改后调用） */
    void refreshMonitorCards();

private:
    /** @brief 为 MasterClient 信号连接到一张卡片 */
    void connectCard(MonitorPage* card);

    Ui::MainWindow* ui;
    DeviceManager* deviceMgr_;
    MasterClient* client_;
    QVector<MonitorPage*> cards_;
    DeviceManagePage* managePage_;
};
