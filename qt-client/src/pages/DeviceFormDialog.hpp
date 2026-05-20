#pragma once

/**
 * @file   DeviceFormDialog.hpp
 * @brief  设备编辑对话框——基本信息 + 测点限值编辑
 * @module qt-client
 */

#include <QDialog>
#include <vector>
#include "storage/DeviceManager.hpp"

namespace Ui { class DeviceFormDialog; }

class DeviceFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit DeviceFormDialog(QWidget* parent = 0);
    ~DeviceFormDialog();

    /** @brief 填充设备信息（编辑模式），测点从 DB 加载 */
    void setDevice(const DeviceInfo& d, DeviceManager* mgr, int deviceId);

    /** @brief 新建模式 */
    void setNewMode();

    /** @brief 获取表单中的设备信息 */
    DeviceInfo deviceInfo() const;

    /** @brief 获取修改后的测点列表 */
    std::vector<PointInfo> modifiedPoints() const;

    /** @brief 获取待删除的测点 ID 列表 */
    std::vector<int> deletedPointIds() const { return deletedPointIds_; }

    /** @brief 获取测点表格中已新增但尚未落库的行（id==0） */
    std::vector<PointInfo> newPoints() const;

    /** @brief 设备 DB ID（编辑模式有效） */
    int deviceId() const { return deviceId_; }

    /** @brief 是否为新建模式 */
    bool isNew() const { return isNew_; }

private slots:
    void onSave();
    void onAddPoint();
    void onDeletePoint();

private:
    void loadPointsToTable(DeviceManager* mgr, int deviceId);

    Ui::DeviceFormDialog* ui;
    DeviceManager* mgr_;
    int deviceId_;
    bool isNew_;
    std::vector<PointInfo> originalPoints_;
    std::vector<int> deletedPointIds_;  ///< 待删除的测点 ID 列表
};
