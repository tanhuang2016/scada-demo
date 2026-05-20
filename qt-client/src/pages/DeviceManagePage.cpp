/**
 * @file   DeviceManagePage.cpp
 * @brief  设备管理页实现——设备 CRUD + 主站热加载通知
 * @module qt-client
 */

#include "pages/DeviceManagePage.hpp"
#include "pages/DeviceFormDialog.hpp"
#include "ui_DeviceManagePage.h"

#include <QMessageBox>
#include <QTcpSocket>

DeviceManagePage::DeviceManagePage(DeviceManager* mgr, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::DeviceManagePage)
    , mgr_(mgr)
{
    ui->setupUi(this);

    ui->deviceTable->setColumnCount(7);

    QObject::connect(ui->addButton, SIGNAL(clicked()), this, SLOT(onAdd()));
    QObject::connect(ui->editButton, SIGNAL(clicked()), this, SLOT(onEdit()));
    QObject::connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(onDelete()));
    QObject::connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(onRefresh()));

    refreshTable();
}

DeviceManagePage::~DeviceManagePage()
{
    delete ui;
}

void DeviceManagePage::refreshTable()
{
    if (mgr_ == NULL) return;

    devices_.clear();
    mgr_->loadAllDevices(devices_);

    ui->deviceTable->setRowCount(static_cast<int>(devices_.size()));

    for (std::size_t i = 0; i < devices_.size(); ++i) {
        const DeviceInfo& d = devices_[i];
        int r = static_cast<int>(i);

        ui->deviceTable->setItem(r, 0, new QTableWidgetItem(
            QString::fromStdString(d.deviceCode)));
        ui->deviceTable->setItem(r, 1, new QTableWidgetItem(
            QString::fromStdString(d.deviceName)));
        ui->deviceTable->setItem(r, 2, new QTableWidgetItem(
            QString::fromStdString(d.ipAddress + ":" + std::to_string(d.port))));
        ui->deviceTable->setItem(r, 3, new QTableWidgetItem(
            QString::fromStdString(d.protocol)));
        ui->deviceTable->setItem(r, 4, new QTableWidgetItem(
            QString::fromStdString(d.stationName)));
        ui->deviceTable->setItem(r, 5, new QTableWidgetItem(
            QString::fromStdString(d.areaName)));
        ui->deviceTable->setItem(r, 6, new QTableWidgetItem(
            d.enabled ? "是" : "否"));
    }

    ui->statusLabel->setText(
        QString("共 %1 台设备").arg(static_cast<int>(devices_.size())));
}

void DeviceManagePage::onAdd()
{
    DeviceFormDialog dlg(this);
    dlg.setNewMode();
    if (dlg.exec() != QDialog::Accepted) return;

    DeviceInfo info = dlg.deviceInfo();
    if (mgr_ != NULL && mgr_->insertDevice(info)) {
        refreshTable();
        notifyMasterReload();
    } else {
        QMessageBox::warning(this, "错误", "新增设备失败");
    }
}

void DeviceManagePage::onEdit()
{
    int row = ui->deviceTable->currentRow();
    if (row < 0 || row >= static_cast<int>(devices_.size())) {
        QMessageBox::information(this, "提示", "请先选择一台设备");
        return;
    }

    const DeviceInfo& d = devices_[static_cast<std::size_t>(row)];

    DeviceFormDialog dlg(this);
    dlg.setDevice(d, mgr_, d.id);
    if (dlg.exec() != QDialog::Accepted) return;

    DeviceInfo info = dlg.deviceInfo();
    if (mgr_ != NULL && mgr_->updateDevice(info)) {
        /* 保存测点限值修改 */
        std::vector<PointInfo> pts = dlg.modifiedPoints();
        for (std::vector<PointInfo>::iterator it = pts.begin();
             it != pts.end(); ++it) {
            mgr_->updatePoint(*it);
        }
        refreshTable();
        notifyMasterReload();
    } else {
        QMessageBox::warning(this, "错误", "更新设备失败");
    }
}

void DeviceManagePage::onDelete()
{
    int row = ui->deviceTable->currentRow();
    if (row < 0 || row >= static_cast<int>(devices_.size())) {
        QMessageBox::information(this, "提示", "请先选择一台设备");
        return;
    }

    const DeviceInfo& d = devices_[static_cast<std::size_t>(row)];
    QString msg = QString("确定要删除设备 %1 及其所有测点吗？")
        .arg(QString::fromStdString(d.deviceCode));

    if (QMessageBox::question(this, "确认删除", msg)
        != QMessageBox::Yes) return;

    if (mgr_ != NULL && mgr_->deleteDevice(d.id)) {
        refreshTable();
        notifyMasterReload();
    } else {
        QMessageBox::warning(this, "错误", "删除失败");
    }
}

void DeviceManagePage::onRefresh()
{
    refreshTable();
}

/*
 * 通过 TCP 连接主站端口 5003，发送 RELOAD 命令通知热加载。
 *
 * 协议：单行文本 "RELOAD\n"，发完即断开。
 * 如果主站不在线（连接失败），静默忽略——配置已落库，
 * 下次主站启动时会自动加载最新配置。
 */
void DeviceManagePage::notifyMasterReload()
{
    QTcpSocket sock;
    sock.connectToHost(QString::fromUtf8("127.0.0.1"), 5003);
    if (sock.waitForConnected(2000)) {
        sock.write("RELOAD\n");
        sock.waitForBytesWritten(2000);
        sock.disconnectFromHost();
        ui->statusLabel->setText(
            QString("共 %1 台设备 — 已通知主站热加载")
            .arg(static_cast<int>(devices_.size())));
    } else {
        ui->statusLabel->setText(
            QString("共 %1 台设备 — 已保存（主站不在线）")
            .arg(static_cast<int>(devices_.size())));
    }
}
