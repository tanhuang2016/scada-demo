#pragma once

/**
 * @file   MainWindow.hpp
 * @brief  主窗口：布局由 ui/MainWindow.ui 定义，本类仅处理业务与状态更新
 */

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

/**
 * @brief SCADA 客户端主窗口（迭代 3 起挂载各业务页面）
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    /** 由 Qt Designer 生成的 UI 类，对应 ui/MainWindow.ui */
    Ui::MainWindow* ui;
};
