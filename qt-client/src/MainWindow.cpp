#include "MainWindow.hpp"

#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include "scada/config_defaults.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("SCADA Demo"));
    resize(960, 640);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    statusLabel_ = new QLabel(
        tr("stub mode — waiting for master-server on port %1")
            .arg(scada::config::kMasterToUiPort),
        central);
    statusLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLabel_);

    setCentralWidget(central);
    statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow() = default;
