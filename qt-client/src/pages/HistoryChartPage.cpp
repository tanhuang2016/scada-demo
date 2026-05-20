/**
 * @file   HistoryChartPage.cpp
 * @brief  历史曲线页——MySQL 查询 + Qt Charts 绘制（动态设备列表）
 * @module qt-client
 */

#include "pages/HistoryChartPage.hpp"
#include "ui_HistoryChartPage.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QDateTime>
#include <QVBoxLayout>
#include <vector>

QT_CHARTS_USE_NAMESPACE

HistoryChartPage::HistoryChartPage(MYSQL* mysql, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::HistoryChartPage)
    , mysql_(mysql)
    , chart_(NULL)
    , voltageSeries_(NULL)
    , currentSeries_(NULL)
    , chartView_(NULL)
{
    ui->setupUi(this);

    /* 从 MySQL 动态加载设备列表 */
    loadDeviceList();

    QObject::connect(ui->queryButton, SIGNAL(clicked()), this, SLOT(onQuery()));

    setupChart();
}

HistoryChartPage::~HistoryChartPage()
{
    delete ui;
}

/*
 * 从 MySQL device 表加载所有启用的设备编码到下拉框。
 */
void HistoryChartPage::loadDeviceList()
{
    if (mysql_ == NULL) return;

    const char* sql = "SELECT device_code FROM device WHERE enabled=1 ORDER BY sort_order";
    if (mysql_query(mysql_, sql) != 0) return;

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (res == NULL) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        if (row[0]) {
            ui->deviceCombo->addItem(QString::fromUtf8(row[0]));
        }
    }
    mysql_free_result(res);
}

void HistoryChartPage::setupChart()
{
    chart_ = new QChart();
    chart_->setTitle(QString::fromUtf8("电压 / 电流 历史曲线"));

    voltageSeries_ = new QLineSeries();
    voltageSeries_->setName(QString::fromUtf8("电压 (V)"));
    voltageSeries_->setPen(QPen(Qt::blue, 1.5));
    chart_->addSeries(voltageSeries_);

    currentSeries_ = new QLineSeries();
    currentSeries_->setName(QString::fromUtf8("电流 (A)"));
    currentSeries_->setPen(QPen(Qt::red, 1.5));
    chart_->addSeries(currentSeries_);

    QDateTimeAxis* axisX = new QDateTimeAxis();
    axisX->setFormat("HH:mm");
    axisX->setTitleText(QString::fromUtf8("时间"));
    chart_->addAxis(axisX, Qt::AlignBottom);
    voltageSeries_->attachAxis(axisX);
    currentSeries_->attachAxis(axisX);

    QValueAxis* axisYV = new QValueAxis();
    axisYV->setTitleText(QString::fromUtf8("电压 V"));
    axisYV->setRange(0, 300);
    chart_->addAxis(axisYV, Qt::AlignLeft);
    voltageSeries_->attachAxis(axisYV);

    QValueAxis* axisYC = new QValueAxis();
    axisYC->setTitleText(QString::fromUtf8("电流 A"));
    axisYC->setRange(0, 50);
    chart_->addAxis(axisYC, Qt::AlignRight);
    currentSeries_->attachAxis(axisYC);

    chartView_ = new QChartView(chart_);
    chartView_->setRenderHint(QPainter::Antialiasing);
    ui->chartLayout->addWidget(chartView_);
}

/*
 * 从 MySQL telemetry 表查询数据并绘制曲线。
 *
 * MySQL TIMESTAMP 列返回格式："2026-05-20 16:06:56"
 * QDateTime::fromString 用 "yyyy-MM-dd HH:mm:ss" 解析。
 *
 * 如果 telemetry 表为空（刚启动或之前 INSERT 失败），曲线无数据点。
 * 需要先运行主站一段时间积累历史数据。
 */
void HistoryChartPage::onQuery()
{
    if (mysql_ == NULL) return;

    QString device = ui->deviceCombo->currentText();
    if (device.isEmpty()) return;

    QString rangeSql;
    int idx = ui->rangeCombo->currentIndex();
    if (idx == 0) rangeSql = "INTERVAL 1 HOUR";
    else if (idx == 1) rangeSql = "INTERVAL 1 DAY";
    else rangeSql = "INTERVAL 7 DAY";

    voltageSeries_->clear();
    currentSeries_->clear();
    double vMin = 9999, vMax = -9999, cMin = 9999, cMax = -9999;
    qint64 tMin = 0, tMax = 0;

    /* 查询电压 */
    {
        std::string sql =
            "SELECT timestamp, value_float FROM telemetry "
            "WHERE device_code='" + device.toStdString() + "' "
            "AND point_code='VOLTAGE' "
            "AND timestamp >= NOW() - " + rangeSql.toStdString() + " "
            "ORDER BY timestamp ASC LIMIT 2000";

        if (mysql_query(mysql_, sql.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(mysql_);
            if (res != NULL) {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != NULL) {
                    if (row[0] && row[1]) {
                        QDateTime dt = QDateTime::fromString(
                            QString::fromUtf8(row[0]), "yyyy-MM-dd HH:mm:ss");
                        if (!dt.isValid()) continue;
                        double val = std::atof(row[1]);
                        qint64 ms = dt.toMSecsSinceEpoch();
                        voltageSeries_->append(ms, val);
                        if (val < vMin) vMin = val;
                        if (val > vMax) vMax = val;
                        if (tMin == 0 || ms < tMin) tMin = ms;
                        if (ms > tMax) tMax = ms;
                    }
                }
                mysql_free_result(res);
            }
        }
    }

    /* 查询电流 */
    {
        std::string sql =
            "SELECT timestamp, value_float FROM telemetry "
            "WHERE device_code='" + device.toStdString() + "' "
            "AND point_code='CURRENT' "
            "AND timestamp >= NOW() - " + rangeSql.toStdString() + " "
            "ORDER BY timestamp ASC LIMIT 2000";

        if (mysql_query(mysql_, sql.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(mysql_);
            if (res != NULL) {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != NULL) {
                    if (row[0] && row[1]) {
                        QDateTime dt = QDateTime::fromString(
                            QString::fromUtf8(row[0]), "yyyy-MM-dd HH:mm:ss");
                        if (!dt.isValid()) continue;
                        double val = std::atof(row[1]);
                        qint64 ms = dt.toMSecsSinceEpoch();
                        currentSeries_->append(ms, val);
                        if (val < cMin) cMin = val;
                        if (val > cMax) cMax = val;
                        if (tMin == 0 || ms < tMin) tMin = ms;
                        if (ms > tMax) tMax = ms;
                    }
                }
                mysql_free_result(res);
            }
        }
    }

    /* 自适应轴范围 */
    QList<QAbstractAxis*> axesX = chart_->axes(Qt::Horizontal);
    if (!axesX.isEmpty() && tMin > 0 && tMax > 0) {
        QDateTimeAxis* ax = qobject_cast<QDateTimeAxis*>(axesX[0]);
        if (ax != NULL) {
            ax->setRange(QDateTime::fromMSecsSinceEpoch(tMin),
                         QDateTime::fromMSecsSinceEpoch(tMax));
        }
    }

    QList<QAbstractAxis*> axesY = chart_->axes(Qt::Vertical, voltageSeries_);
    if (!axesY.isEmpty() && vMin < vMax) {
        QValueAxis* ax = qobject_cast<QValueAxis*>(axesY[0]);
        if (ax != NULL) {
            double margin = (vMax - vMin) * 0.1 + 5.0;
            ax->setRange(vMin - margin, vMax + margin);
        }
    }
    if (cMin < cMax) {
        QList<QAbstractAxis*> axesYC = chart_->axes(Qt::Vertical, currentSeries_);
        if (!axesYC.isEmpty()) {
            QValueAxis* ax = qobject_cast<QValueAxis*>(axesYC[0]);
            if (ax != NULL) {
                double margin = (cMax - cMin) * 0.1 + 2.0;
                ax->setRange(cMin - margin, cMax + margin);
            }
        }
    }
}
