/**
 * @file   DeviceFormDialog.cpp
 * @brief  设备编辑对话框实现
 * @module qt-client
 */

#include "pages/DeviceFormDialog.hpp"
#include "ui_DeviceFormDialog.h"

#include <QMessageBox>

DeviceFormDialog::DeviceFormDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DeviceFormDialog)
    , mgr_(NULL)
    , deviceId_(0)
    , isNew_(true)
{
    ui->setupUi(this);

    ui->pointsTable->setColumnCount(6);

    QObject::connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(onSave()));
    QObject::connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

DeviceFormDialog::~DeviceFormDialog()
{
    delete ui;
}

void DeviceFormDialog::setDevice(const DeviceInfo& d, DeviceManager* mgr, int deviceId)
{
    isNew_ = false;
    deviceId_ = deviceId;
    mgr_ = mgr;

    ui->editCode->setText(QString::fromStdString(d.deviceCode));
    ui->editName->setText(QString::fromStdString(d.deviceName));
    ui->editType->setText(QString::fromStdString(d.deviceType));
    ui->editStation->setText(QString::fromStdString(d.stationName));
    ui->editArea->setText(QString::fromStdString(d.areaName));
    ui->editIP->setText(QString::fromStdString(d.ipAddress));
    ui->editPort->setValue(d.port);
    ui->comboProtocol->setCurrentText(QString::fromStdString(d.protocol));
    ui->editCA->setValue(d.commonAddress);
    ui->checkEnabled->setChecked(d.enabled);

    if (mgr != NULL) {
        loadPointsToTable(mgr, deviceId);
    }
}

void DeviceFormDialog::setNewMode()
{
    isNew_ = true;
    deviceId_ = 0;
}

void DeviceFormDialog::onSave()
{
    if (ui->editCode->text().isEmpty()) {
        QMessageBox::warning(this, "验证失败", "设备编码不能为空");
        return;
    }
    accept();
}

DeviceInfo DeviceFormDialog::deviceInfo() const
{
    DeviceInfo d;
    d.id = deviceId_;
    d.deviceCode = ui->editCode->text().toStdString();
    d.deviceName = ui->editName->text().toStdString();
    d.deviceType = ui->editType->text().toStdString();
    d.stationName = ui->editStation->text().toStdString();
    d.areaName = ui->editArea->text().toStdString();
    d.ipAddress = ui->editIP->text().toStdString();
    d.port = ui->editPort->value();
    d.protocol = ui->comboProtocol->currentText().toStdString();
    d.commonAddress = ui->editCA->value();
    d.connectTimeoutSec = 30;
    d.reconnectIntervalSec = 5;
    d.enabled = ui->checkEnabled->isChecked();
    return d;
}

std::vector<PointInfo> DeviceFormDialog::modifiedPoints() const
{
    std::vector<PointInfo> result;
    int rows = ui->pointsTable->rowCount();

    for (int r = 0; r < rows; ++r) {
        PointInfo p;
        /* 从 originalPoints_ 获取基础数据 */
        if (r < static_cast<int>(originalPoints_.size())) {
            p = originalPoints_[static_cast<std::size_t>(r)];
        }

        /* 读取上限 */
        QTableWidgetItem* hi = ui->pointsTable->item(r, 4);
        if (hi != NULL) {
            p.limitHigh = hi->text().toDouble();
        }

        /* 读取下限 */
        QTableWidgetItem* lo = ui->pointsTable->item(r, 5);
        if (lo != NULL) {
            p.limitLow = lo->text().toDouble();
        }

        result.push_back(p);
    }
    return result;
}

void DeviceFormDialog::loadPointsToTable(DeviceManager* mgr, int deviceId)
{
    originalPoints_.clear();
    if (mgr == NULL) return;

    mgr->loadPoints(deviceId, originalPoints_);

    ui->pointsTable->setRowCount(static_cast<int>(originalPoints_.size()));

    for (std::size_t i = 0; i < originalPoints_.size(); ++i) {
        const PointInfo& p = originalPoints_[i];
        int r = static_cast<int>(i);

        ui->pointsTable->setItem(r, 0,
            new QTableWidgetItem(QString::number(p.ioa)));
        ui->pointsTable->setItem(r, 1,
            new QTableWidgetItem(QString::fromStdString(p.pointCode)));
        ui->pointsTable->setItem(r, 2,
            new QTableWidgetItem(QString::fromStdString(p.pointName)));
        ui->pointsTable->setItem(r, 3,
            new QTableWidgetItem(QString::fromStdString(p.pointType)));
        /* 限值可编辑 */
        QTableWidgetItem* hi = new QTableWidgetItem(
            QString::number(p.limitHigh, 'f', 2));
        ui->pointsTable->setItem(r, 4, hi);
        QTableWidgetItem* lo = new QTableWidgetItem(
            QString::number(p.limitLow, 'f', 2));
        ui->pointsTable->setItem(r, 5, lo);
    }
}
