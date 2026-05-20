/**
 * @file   OperationLogPage.cpp
 * @brief  操作日志页——从 MySQL operation_log 读取并展示
 * @module qt-client
 */

#include "pages/OperationLogPage.hpp"
#include "ui_OperationLogPage.h"

OperationLogPage::OperationLogPage(MYSQL* mysql, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::OperationLogPage)
    , mysql_(mysql)
{
    ui->setupUi(this);
    ui->logTable->setColumnCount(6);

    QObject::connect(ui->refreshButton, SIGNAL(clicked()),
                     this, SLOT(refreshTable()));

    refreshTable();
}

OperationLogPage::~OperationLogPage()
{
    delete ui;
}

void OperationLogPage::refreshTable()
{
    if (mysql_ == NULL) return;

    const char* sql =
        "SELECT operated_at, device_code, operator, operation_desc, "
        "result, failure_reason FROM operation_log "
        "ORDER BY operated_at DESC LIMIT 100";

    if (mysql_query(mysql_, sql) != 0) return;

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (res == NULL) return;

    int rows = static_cast<int>(mysql_num_rows(res));
    ui->logTable->setRowCount(rows);

    MYSQL_ROW row;
    for (int r = 0; r < rows; ++r) {
        row = mysql_fetch_row(res);
        if (row == NULL) break;

        ui->logTable->setItem(r, 0, new QTableWidgetItem(row[0] ? row[0] : ""));
        ui->logTable->setItem(r, 1, new QTableWidgetItem(row[1] ? row[1] : ""));
        ui->logTable->setItem(r, 2, new QTableWidgetItem(row[2] ? row[2] : ""));
        ui->logTable->setItem(r, 3, new QTableWidgetItem(row[3] ? row[3] : ""));
        ui->logTable->setItem(r, 4, new QTableWidgetItem(row[4] ? row[4] : ""));

        QString reason = row[5] ? QString::fromUtf8(row[5]) : QString();
        ui->logTable->setItem(r, 5, new QTableWidgetItem(reason));
    }

    mysql_free_result(res);
}
