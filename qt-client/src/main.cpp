/**
 * @file   main.cpp
 * @brief  Qt 客户端入口：登录 → 主窗口
 * @module qt-client
 */

#include "MainWindow.hpp"
#include "pages/LoginDialog.hpp"

#include <QApplication>
#include <QDialog>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("SCADA");
    app.setApplicationName("qt-client");

    /* 登录对话框 */
    LoginDialog login;
    if (login.exec() != QDialog::Accepted) {
        return 0;
    }

    /* 登录成功，进入主窗口 */
    MainWindow window;
    window.show();

    return app.exec();
}
