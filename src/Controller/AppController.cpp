#include "AppController.hpp"
#include "../Core/Capture/InterfaceManager.hpp"
#include <QDebug>
#include <QDateTime>

AppController::AppController(MainWindow *mainWindow, QObject *parent)
    : QObject(parent),
    m_mainWindow(mainWindow)
// m_currentDisplayFilter("") // <-- ĐÃ XÓA
{
    // --- Khởi tạo Core ---
    m_captureEngine = new CaptureEngine(this);

    // --- THÊM MỚI: Khởi tạo Bộ máy Lọc ---
    m_filterEngine = new DisplayFilterEngine(this);

    loadInterfaces();

    // --- Connect signal từ MainWindow (forward từ Page) ---
    connect(m_mainWindow, &MainWindow::interfaceSelected, this, &AppController::onInterfaceSelected);
    connect(m_mainWindow, &MainWindow::openFileRequested, this, &AppController::onOpenFileRequested);
    connect(m_mainWindow, &MainWindow::onRestartCaptureClicked, this, &AppController::onRestartCaptureClicked);
    connect(m_mainWindow, &MainWindow::onStopCaptureClicked, this, &AppController::onStopCaptureClicked);
    connect(m_mainWindow, &MainWindow::onPauseCaptureClicked, this, &AppController::onPauseCaptureClicked);
    connect(m_mainWindow, &MainWindow::onApplyFilterClicked, this, &AppController::onApplyFilterClicked);

    // --- Connect signal từ Core ---
    connect(m_captureEngine, &CaptureEngine::packetCaptured, this, &AppController::onPacketCaptured);

    // --- Connect TÍN HIỆU (Signal) của AppController VỚI (Slot) của MainWindow ---
    connect(this, &AppController::displayNewPacket, m_mainWindow, &MainWindow::addPacketToTable);
    connect(this, &AppController::clearPacketTable, m_mainWindow, &MainWindow::clearPacketTable);

    // --- THÊM MỚI: Kết nối tín hiệu Lỗi Filter với MainWindow ---
    // (Bạn sẽ cần thêm slot 'showFilterError' vào MainWindow)
    connect(this, &AppController::displayFilterError,
            m_mainWindow, &MainWindow::showFilterError);
}

void AppController::onInterfaceSelected(const QString &interfaceName, const QString &filterText)
{
    qDebug() << "Interface selected:" << interfaceName << "Filter:" << filterText;

    m_allPackets.clear();
    m_filterEngine->setFilter(""); // <-- Reset bộ lọc BPF
    emit clearPacketTable();

    m_captureEngine->setInterface(interfaceName);
    m_captureEngine->setCaptureFilter(filterText);
    m_captureEngine->startCapture();

    m_mainWindow->showCapturePage();
}

void AppController::onOpenFileRequested()
{
    qDebug() << "Open file requested";
    // TODO: mở file pcap và load vào Core → cập nhật UI
}

void AppController::onRestartCaptureClicked()
{
    qDebug() << "Restart capture";

    m_allPackets.clear();
    m_filterEngine->setFilter(""); // <-- Reset bộ lọc BPF
    emit clearPacketTable();

    m_captureEngine->stopCapture();
    m_captureEngine->startCapture();
}

void AppController::onStopCaptureClicked()
{
    qDebug() << "Stop capture";
    m_captureEngine->stopCapture();
}

void AppController::onPauseCaptureClicked()
{
    qDebug() << "Pause/Resume capture";
    if (m_captureEngine->isPaused()) {
        m_captureEngine->resumeCapture();
    } else {
        m_captureEngine->pauseCapture();
    }
}

/**
 * @brief (ĐÃ THAY ĐỔI) Được gọi khi nhấn "Apply" trên CapturePage
 */
void AppController::onApplyFilterClicked(const QString &filterText)
{
    qDebug() << "Apply BPF display filter:" << filterText;

    // 1. Biên dịch (compile) filter mới bằng BPF
    if (!m_filterEngine->setFilter(filterText))
    {
        // 2. Nếu lỗi cú pháp, gửi tín hiệu lỗi lên UI
        emit displayFilterError(m_filterEngine->getLastError());

        // (Chúng ta xóa bộ lọc nếu nó bị lỗi)
        m_filterEngine->setFilter("");
    }

    // 3. Lọc lại toàn bộ bảng
    refreshFullDisplay();
}

/**
 * @brief (ĐÃ THAY ĐỔI) Được gọi khi Core bắt được 1 gói tin
 */
void AppController::onPacketCaptured(const PacketData &packet)
{
    // 1. Luôn luôn lưu vào danh sách chính
    m_allPackets.append(packet);

    // 2. Chỉ gửi tín hiệu hiển thị nếu nó khớp với bộ lọc BPF
    if (m_filterEngine->packetMatches(packet))
    {
        emit displayNewPacket(packet);
    }
}

/**
 * @brief (ĐÃ THAY ĐỔI) Lọc lại toàn bộ danh sách
 */
void AppController::refreshFullDisplay()
{
    // 1. Yêu cầu UI xóa sạch
    emit clearPacketTable();

    // 2. Lặp lại danh sách chính
    for (const PacketData &packet : m_allPackets) {
        // 3. Chỉ thêm lại những gói tin khớp với bộ lọc BPF
        if (m_filterEngine->packetMatches(packet)) {
            emit displayNewPacket(packet);
        }
    }
}

void AppController::loadInterfaces()
{
    auto devices = InterfaceManager::getDevices();
    m_mainWindow->setDevices(devices);
}
