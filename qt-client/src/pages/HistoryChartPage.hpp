#pragma once

/**
 * @file   HistoryChartPage.hpp
 * @brief  历史曲线页——从 MySQL telemetry 表查询并绘制电压/电流曲线
 * @module qt-client
 */

#include <QWidget>
#include <mysql.h>

namespace Ui { class HistoryChartPage; }
namespace QtCharts { class QChart; class QLineSeries; class QChartView; class QDateTimeAxis; class QValueAxis; }

class HistoryChartPage : public QWidget {
    Q_OBJECT

public:
    explicit HistoryChartPage(MYSQL* mysql, QWidget* parent = 0);
    ~HistoryChartPage();

private slots:
    void onQuery();

private:
    void loadDeviceList();
    void setupChart();

    Ui::HistoryChartPage* ui;
    MYSQL* mysql_;
    QtCharts::QChart* chart_;
    QtCharts::QLineSeries* voltageSeries_;
    QtCharts::QLineSeries* currentSeries_;
    QtCharts::QChartView* chartView_;
};
