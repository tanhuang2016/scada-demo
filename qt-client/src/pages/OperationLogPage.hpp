#pragma once

/**
 * @file   OperationLogPage.hpp
 * @brief  操作日志页——从 MySQL operation_log 表读取并展示
 * @module qt-client
 */

#include <QWidget>
#include <mysql.h>

namespace Ui { class OperationLogPage; }

class OperationLogPage : public QWidget {
    Q_OBJECT

public:
    explicit OperationLogPage(MYSQL* mysql, QWidget* parent = 0);
    ~OperationLogPage();

public slots:
    void refreshTable();

private:
    Ui::OperationLogPage* ui;
    MYSQL* mysql_;
};
