/**
 * @file   AlarmPage.cpp
 * @brief  告警列表页——从 MySQL alarm 表读取展示
 * @module qt-client
 */

#include "pages/AlarmPage.hpp"
#include "ui_AlarmPage.h"

AlarmPage::AlarmPage(MYSQL* mysql, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::AlarmPage)
    , mysql_(mysql)
{
    ui->setupUi(this);
    ui->alarmTable->setColumnCount(5);

    QObject::connect(ui->refreshButton, SIGNAL(clicked()),
                     this, SLOT(refreshTable()));

    refreshTable();
}

AlarmPage::~AlarmPage()
{
    delete ui;
}

void AlarmPage::refreshTable()
{
    if (mysql_ == NULL) return;

    const char* sql =
        "SELECT occurred_at, device_code, alarm_type, value_current, state "
        "FROM alarm ORDER BY occurred_at DESC LIMIT 200";

    if (mysql_query(mysql_, sql) != 0) return;

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (res == NULL) return;

    int rows = static_cast<int>(mysql_num_rows(res));
    ui->alarmTable->setRowCount(rows);

    MYSQL_ROW row;
    for (int r = 0; r < rows; ++r) {
        row = mysql_fetch_row(res);
        if (row == NULL) break;

        ui->alarmTable->setItem(r, 0, new QTableWidgetItem(row[0] ? row[0] : ""));
        ui->alarmTable->setItem(r, 1, new QTableWidgetItem(row[1] ? row[1] : ""));
        ui->alarmTable->setItem(r, 2, new QTableWidgetItem(row[2] ? row[2] : ""));

        /* 电流/电压值 */
        if (row[3]) {
            QString val = QString::number(std::atof(row[3]), 'f', 1);
            if (std::string(row[2]) == "VOLTAGE_HIGH" || std::string(row[2]) == "VOLTAGE_LOW") {
                val += " V";
            }
            ui->alarmTable->setItem(r, 3, new QTableWidgetItem(val));
        } else {
            ui->alarmTable->setItem(r, 3, new QTableWidgetItem(""));
        }

        /* 状态着色 */
        QTableWidgetItem* stateItem = new QTableWidgetItem(row[4] ? row[4] : "");
        if (row[4] && std::string(row[4]) == "ACTIVE") {
            stateItem->setForeground(Qt::red);
        }
        ui->alarmTable->setItem(r, 4, stateItem);
    }

    mysql_free_result(res);
}
