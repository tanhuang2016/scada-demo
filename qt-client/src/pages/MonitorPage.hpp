#pragma once

/**
 * @file   MonitorPage.hpp
 * @brief  实时监控页——展示单设备遥测卡片（电压/电流/开关/时间）
 * @module qt-client
 */

#include <QWidget>
#include <ctime>

// 前向声明
namespace Ui {
class MonitorPage;
}

namespace scada {
struct Telemetry;
}  // namespace scada

/**
 * @brief 实时监控页 Widget，对应 ui/MonitorPage.ui
 *
 * 接收 MasterClient 的遥测信号，更新卡片上的电压/电流/开关/时间。
 * 连接状态变化时更新状态标签颜色（绿色=在线，灰色=离线）。
 */
class MonitorPage : public QWidget {
    Q_OBJECT

public:
    explicit MonitorPage(QWidget* parent = 0);
    ~MonitorPage();

public slots:
    /** 收到一条遥测数据，更新卡片显示 */
    void onTelemetry(const scada::Telemetry& telemetry);

    /** 连接状态变化，更新状态标签 */
    void onConnectionStateChanged(bool connected);

private:
    Ui::MonitorPage* ui;
    std::time_t lastUpdateTime_;
};

// 注册 Telemetry 元类型（在 MonitorPage.cpp 中 qRegisterMetaType）
#include "scada/types.hpp"
Q_DECLARE_METATYPE(scada::Telemetry)
