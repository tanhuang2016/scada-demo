#pragma once

#include <QMainWindow>

class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    QLabel* statusLabel_ = nullptr;
};
