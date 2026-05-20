#pragma once

/**
 * @file   AlarmPage.hpp
 * @brief  告警列表页——从 MySQL alarm 表读取并展示
 * @module qt-client
 */

#include <QWidget>
#include <mysql.h>

namespace Ui { class AlarmPage; }

class AlarmPage : public QWidget {
    Q_OBJECT

public:
    explicit AlarmPage(MYSQL* mysql, QWidget* parent = 0);
    ~AlarmPage();

public slots:
    void refreshTable();

private:
    Ui::AlarmPage* ui;
    MYSQL* mysql_;
};
