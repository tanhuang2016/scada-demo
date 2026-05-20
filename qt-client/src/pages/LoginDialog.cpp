/**
 * @file   LoginDialog.cpp
 * @brief  登录对话框实现
 * @module qt-client
 */

#include "pages/LoginDialog.hpp"
#include "ui_LoginDialog.h"

#include <QMessageBox>
#include <QSettings>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    QObject::connect(ui->loginButton, SIGNAL(clicked()), this, SLOT(onLogin()));
    loadPrefs();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::loadPrefs()
{
    QSettings settings("SCADA", "qt-client");
    QString lastUser = settings.value("login/lastUser", "").toString();
    bool remember = settings.value("login/remember", false).toBool();

    if (!lastUser.isEmpty()) {
        ui->editUser->setText(lastUser);
        ui->checkRemember->setChecked(remember);
        ui->editPass->setFocus();
    }
}

void LoginDialog::savePrefs()
{
    QSettings settings("SCADA", "qt-client");
    if (ui->checkRemember->isChecked()) {
        settings.setValue("login/lastUser", ui->editUser->text());
        settings.setValue("login/remember", true);
    } else {
        settings.remove("login/lastUser");
        settings.setValue("login/remember", false);
    }
}

void LoginDialog::onLogin()
{
    QString user = ui->editUser->text().trimmed();
    QString pass = ui->editPass->text();

    /* 简单硬编码验证 */
    if (user == "admin" && pass == "123456") {
        savePrefs();
        accept();
    } else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误");
        ui->editPass->clear();
        ui->editPass->setFocus();
    }
}
