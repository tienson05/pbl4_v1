#include "MainWindow.hpp"
#include "Header/HeaderWidget.hpp"
#include "Pages/WelcomePage.hpp"
#include "Pages/CapturePage.hpp"
#include "Widgets/PacketTable.hpp"

#include <QVBoxLayout>
#include <QDebug> // (Để debug)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // --- Header ---
    header = new HeaderWidget(this);

    // --- QStackedWidget & Pages ---
    stack = new QStackedWidget(this);
    welcomePage = new WelcomePage(this);
    capturePage = new CapturePage(this); // <-- Tên biến là 'capturePage'

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
    setWindowTitle("WiresharkMini");
    resize(1280, 800);

    // --- Kết nối signal cục bộ (cho Header) ---
    connect(header, &HeaderWidget::minimizeRequested, this, &MainWindow::onMinimizeRequested);
    connect(header, &HeaderWidget::maximizeRequested, this, &MainWindow::onMaximizeRequested);
    connect(header, &HeaderWidget::closeRequested, this, &MainWindow::onCloseRequested);
    connect(header, &HeaderWidget::saveFileRequested, // <-- Tín hiệu từ HeaderWidget
            this, &MainWindow::saveFileRequested); // <-- Chuyển tiếp (forward) lên MainWindow
    connect(header, &HeaderWidget::openFileRequested,
            this, &MainWindow::openFileRequested);

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

// --- SLOTS CÔNG KHAI (do AppController gọi) ---

/**
 * @brief (ĐÃ SỬA) Chuyển tiếp lệnh 'addPacket' xuống PacketTable
 */
void MainWindow::addPacketToTable(const PacketData &packet)
{
    // Tên biến đúng là 'capturePage' (không có 'm_')
    if (capturePage) {
        capturePage->packetTable->addPacket(packet);
    }
}

/**
 * @brief (ĐÃ SỬA) Chuyển tiếp lệnh 'clearData' xuống PacketTable
 */
void MainWindow::clearPacketTable()
{
    if (capturePage) {
        capturePage->packetTable->clearData();
    }
}
/**
 * @brief (THÊM MỚI) Hàm này được AppController gọi khi cú pháp BPF bị sai.
 */
void MainWindow::showFilterError(const QString &errorText)
{
    QMessageBox::warning(this, "Display Filter Error",
                         "The filter syntax is invalid. Please check and try again.\n\n"
                         "Error: " + errorText);
}
// --- Hàm tiện ích (do AppController gọi) ---
void MainWindow::setDevices(const QVector<QPair<QString, QString>> &devices)
{
    // (Giả sử WelcomePage có hàm này)
    welcomePage->setDevices(devices);
}


// --- Các slot xử lý cục bộ (cho Header) ---
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
