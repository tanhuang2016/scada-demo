#pragma once

/**
 * @file   DeviceManagePage.hpp
 * @brief  设备管理页——列表展示 + 增删改 + 通知主站热加载
 * @module qt-client
 */

#include <QWidget>
#include <vector>

#include "storage/DeviceManager.hpp"

namespace Ui { class DeviceManagePage; }

class DeviceManagePage : public QWidget {
    Q_OBJECT

public:
    explicit DeviceManagePage(DeviceManager* mgr, QWidget* parent = 0);
    ~DeviceManagePage();

    /** @brief 加载设备列表到表格 */
    void refreshTable();

signals:
    /** @brief 设备配置已变更（增删改后触发，通知 MainWindow 刷新卡片） */
    void devicesChanged();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onRefresh();

private:
    /** @brief 通知主站热加载（TCP 端口 5003，发送 RELOAD） */
    void notifyMasterReload();

    Ui::DeviceManagePage* ui;
    DeviceManager* mgr_;
    std::vector<DeviceInfo> devices_;
};
