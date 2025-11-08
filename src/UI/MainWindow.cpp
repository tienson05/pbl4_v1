#include "MainWindow.hpp"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // --- Header ---
    header = new HeaderWidget(this);

    // --- QStackedWidget & Pages ---
    stack = new QStackedWidget(this);
    welcomePage = new WelcomePage(this);
    capturePage = new CapturePage(this);

    stack->addWidget(welcomePage);
    stack->addWidget(capturePage);

    // --- Layout chính ---
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(header);
    mainLayout->addWidget(stack);

    setCentralWidget(central);

    // --- Kết nối signal cục bộ ---
    connect(header, &HeaderWidget::minimizeRequested, this, &MainWindow::onMinimizeRequested);
    connect(header, &HeaderWidget::maximizeRequested, this, &MainWindow::onMaximizeRequested);
    connect(header, &HeaderWidget::closeRequested, this, &MainWindow::onCloseRequested);

    // --- Forward signal từ WelcomePage sang Controller ---
    connect(welcomePage, &WelcomePage::interfaceSelected,
            this, &MainWindow::interfaceSelected);
    connect(welcomePage, &WelcomePage::openFileRequested,
            this, &MainWindow::openFileRequested);

    // --- Forward signal từ CapturePage sang Controller ---
    connect(capturePage, &CapturePage::onRestartCaptureClicked,
            this, &MainWindow::onRestartCaptureClicked);
    connect(capturePage, &CapturePage::onStopCaptureClicked,
            this, &MainWindow::onStopCaptureClicked);
    connect(capturePage, &CapturePage::onPauseCaptureClicked,
            this, &MainWindow::onPauseCaptureClicked);
    connect(capturePage, &CapturePage::onApplyFilterClicked,
            this, &MainWindow::onApplyFilterClicked);

    // Mặc định hiển thị WelcomePage
    showWelcomePage();
}

// --- Các hàm chuyển trang ---
void MainWindow::showWelcomePage() {
    stack->setCurrentWidget(welcomePage);
}

void MainWindow::showCapturePage() {
    stack->setCurrentWidget(capturePage);
}

// --- Các slot xử lý cục bộ ---
void MainWindow::onMinimizeRequested() {
    showMinimized();
}

void MainWindow::onMaximizeRequested() {
    if(isMaximized())
        showNormal();
    else
        showMaximized();
}

void MainWindow::onCloseRequested() {
    close();
}

void MainWindow::addPacketToTable(const PacketData &packet)
{
    capturePage->packetTable->addPacket(packet);
}
void MainWindow::setDevices(const QVector<QPair<QString, QString>> &devices)
{
    welcomePage->setDevices(devices);
}
