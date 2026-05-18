/**
 * @file   main.cpp
 * @brief  Qt 客户端入口：创建 QApplication 与主窗口
 */

#include "MainWindow.hpp"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
