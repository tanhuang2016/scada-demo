/**
 * @file   DeviceFormDialog.cpp
 * @brief  设备编辑对话框——基本信息 + 测点增删改
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
    QObject::connect(ui->addPointBtn, SIGNAL(clicked()), this, SLOT(onAddPoint()));
    QObject::connect(ui->delPointBtn, SIGNAL(clicked()), this, SLOT(onDeletePoint()));
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
    originalPoints_.clear();
    deletedPointIds_.clear();

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
    originalPoints_.clear();
    deletedPointIds_.clear();
    ui->pointsTable->setRowCount(0);
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

        /* 读取各列（新增行 originalPoints_ 无对应条目，id==0） */
        QTableWidgetItem* item1 = ui->pointsTable->item(r, 0);
        if (item1 != NULL) p.ioa = item1->text().toInt();
        QTableWidgetItem* item2 = ui->pointsTable->item(r, 1);
        if (item2 != NULL) p.pointCode = item2->text().toStdString();
        QTableWidgetItem* item3 = ui->pointsTable->item(r, 2);
        if (item3 != NULL) p.pointName = item3->text().toStdString();
        QTableWidgetItem* item4 = ui->pointsTable->item(r, 3);
        if (item4 != NULL) p.pointType = item4->text().toStdString();
        QTableWidgetItem* hi = ui->pointsTable->item(r, 4);
        if (hi != NULL) p.limitHigh = hi->text().toDouble();
        QTableWidgetItem* lo = ui->pointsTable->item(r, 5);
        if (lo != NULL) p.limitLow = lo->text().toDouble();

        result.push_back(p);
    }
    return result;
}

std::vector<PointInfo> DeviceFormDialog::newPoints() const
{
    std::vector<PointInfo> result;
    int rows = ui->pointsTable->rowCount();

    /* id==0 的行即为新增 */
    for (int r = 0; r < rows; ++r) {
        if (r < static_cast<int>(originalPoints_.size())) {
            if (originalPoints_[static_cast<std::size_t>(r)].id != 0) continue;
        }
        PointInfo p;
        QTableWidgetItem* item1 = ui->pointsTable->item(r, 0);
        if (item1 != NULL) p.ioa = item1->text().toInt();
        QTableWidgetItem* item2 = ui->pointsTable->item(r, 1);
        if (item2 != NULL) p.pointCode = item2->text().toStdString();
        QTableWidgetItem* item3 = ui->pointsTable->item(r, 2);
        if (item3 != NULL) p.pointName = item3->text().toStdString();
        QTableWidgetItem* item4 = ui->pointsTable->item(r, 3);
        if (item4 != NULL) p.pointType = item4->text().toStdString();
        QTableWidgetItem* hi = ui->pointsTable->item(r, 4);
        if (hi != NULL) p.limitHigh = hi->text().toDouble();
        QTableWidgetItem* lo = ui->pointsTable->item(r, 5);
        if (lo != NULL) p.limitLow = lo->text().toDouble();
        if (!p.pointCode.empty()) {
            result.push_back(p);
        }
    }
    return result;
}

/*
 * 添加一行空白测点。
 * 用户可编辑 IOA/编码/名称/类型/限值，保存时写入 MySQL。
 */
void DeviceFormDialog::onAddPoint()
{
    int r = ui->pointsTable->rowCount();
    ui->pointsTable->setRowCount(r + 1);
    /* 默认值 */
    ui->pointsTable->setItem(r, 0, new QTableWidgetItem(QString::number(4000 + r)));
    ui->pointsTable->setItem(r, 1, new QTableWidgetItem(QString::fromUtf8("NEW")));
    ui->pointsTable->setItem(r, 2, new QTableWidgetItem(QString::fromUtf8("新测点")));
    ui->pointsTable->setItem(r, 3, new QTableWidgetItem(QString::fromUtf8("YC")));
    ui->pointsTable->setItem(r, 4, new QTableWidgetItem(QString::number(100.0, 'f', 2)));
    ui->pointsTable->setItem(r, 5, new QTableWidgetItem(QString::number(0.0, 'f', 2)));
}

/*
 * 删除选中行。
 * 如果行对应的测点已在数据库中（originalPoints_ 有记录），加入 deletedPointIds_ 待删除。
 */
void DeviceFormDialog::onDeletePoint()
{
    int r = ui->pointsTable->currentRow();
    if (r < 0) {
        QMessageBox::information(this, "提示", "请先选择一个测点");
        return;
    }

    /* 如果是数据库已有测点，标记待删除 */
    if (r < static_cast<int>(originalPoints_.size())) {
        int pid = originalPoints_[static_cast<std::size_t>(r)].id;
        if (pid > 0) {
            deletedPointIds_.push_back(pid);
        }
    }

    /* 从 originalPoints_ 中移除对应项 */
    if (r < static_cast<int>(originalPoints_.size())) {
        originalPoints_.erase(originalPoints_.begin() + r);
    }

    ui->pointsTable->removeRow(r);
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
        QTableWidgetItem* hi = new QTableWidgetItem(
            QString::number(p.limitHigh, 'f', 2));
        ui->pointsTable->setItem(r, 4, hi);
        QTableWidgetItem* lo = new QTableWidgetItem(
            QString::number(p.limitLow, 'f', 2));
        ui->pointsTable->setItem(r, 5, lo);
    }
}
