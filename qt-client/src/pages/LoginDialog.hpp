#pragma once

/**
 * @file   LoginDialog.hpp
 * @brief  登录对话框——admin/123456 验证 + SQLite 记住用户名
 * @module qt-client
 */

#include <QDialog>

namespace Ui { class LoginDialog; }

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = 0);
    ~LoginDialog();

private slots:
    void onLogin();

private:
    /** @brief 加载 SQLite 保存的用户名和偏好 */
    void loadPrefs();
    /** @brief 保存用户名到 SQLite */
    void savePrefs();

    Ui::LoginDialog* ui;
};
