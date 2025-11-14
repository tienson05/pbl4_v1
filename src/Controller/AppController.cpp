#include "AppController.hpp"
#include "../Core/Capture/InterfaceManager.hpp"
#include <QDebug>
#include <QDateTime>
#include <QThread> // <-- THÊM MỚI: Cần cho luồng lọc

AppController::AppController(MainWindow *mainWindow, QObject *parent)
    : QObject(parent),
    m_mainWindow(mainWindow)
{
    // --- Khởi tạo Core ---
    m_captureEngine = new CaptureEngine(this);

    // --- Khởi tạo Bộ máy Lọc ---
    m_filterEngine = new DisplayFilterEngine(this);

    loadInterfaces();

    // --- Connect signal từ MainWindow (forward từ Page) ---
    connect(m_mainWindow, &MainWindow::interfaceSelected, this, &AppController::onInterfaceSelected);
    connect(m_mainWindow, &MainWindow::openFileRequested, this, &AppController::onOpenFileRequested);
    connect(m_mainWindow, &MainWindow::onRestartCaptureClicked, this, &AppController::onRestartCaptureClicked);
    connect(m_mainWindow, &MainWindow::onStopCaptureClicked, this, &AppController::onStopCaptureClicked);
    connect(m_mainWindow, &MainWindow::onPauseCaptureClicked, this, &AppController::onPauseCaptureClicked);
    connect(m_mainWindow, &MainWindow::onApplyFilterClicked, this, &AppController::onApplyFilterClicked);

    // --- (ĐÃ THAY ĐỔI) Kết nối signal từ Core ---
    // (Kết nối signal "lô" packetsCaptured VỚI slot "lô" onPacketsCaptured)
    connect(m_captureEngine, &CaptureEngine::packetsCaptured,
            this, &AppController::onPacketsCaptured);

    // --- Connect TÍN HIỆU (Signal) của AppController VỚI (Slot) của MainWindow ---
    connect(this, &AppController::displayNewPacket, m_mainWindow, &MainWindow::addPacketToTable);
    connect(this, &AppController::clearPacketTable, m_mainWindow, &MainWindow::clearPacketTable);

    // --- THÊM MỚI: Kết nối tín hiệu LÔ ĐÃ LỌC ---
    // (Chúng ta sẽ cần thêm slot 'addPacketsToTable' (có 's') vào MainWindow)
    connect(this, &AppController::displayFilteredPackets,
            m_mainWindow, &MainWindow::addPacketsToTable);

    connect(this, &AppController::displayFilterError, m_mainWindow, &MainWindow::showFilterError);
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
        m_filterEngine->setFilter("");
    }

    // 3. Lọc lại toàn bộ bảng (sẽ chạy trên luồng MỚI)
    refreshFullDisplay();
}

/**
 * @brief (ĐÃ THAY ĐỔI) Được gọi khi Core bắt được MỘT LÔ gói tin
 */
void AppController::onPacketsCaptured(const QList<PacketData> &packets)
{
    // Tạo một lô (batch) mới chỉ chứa các gói tin đã lọc
    QList<PacketData> filteredBatch;

    for (const PacketData &packet : packets)
    {
        // 1. Luôn luôn lưu vào danh sách chính
        m_allPackets.append(packet);

        // 2. Chỉ thêm vào lô mới nếu nó khớp
        if (m_filterEngine->packetMatches(packet))
        {
            filteredBatch.append(packet);
        }
    }

    // 3. Gửi LÔ ĐÃ LỌC lên UI (nếu có)
    if (!filteredBatch.isEmpty()) {
        emit displayFilteredPackets(filteredBatch);
    }
}

/**
 * @brief (ĐÃ THAY ĐỔI HOÀN TOÀN) Chạy việc lọc trên luồng nền
 */
void AppController::refreshFullDisplay()
{
    // 0. Dừng luồng lọc cũ (nếu nó đang chạy)
    if (m_filterThread && m_filterThread->isRunning()) {
        m_filterThread->requestInterruption(); // Yêu cầu dừng
        m_filterThread->quit();
        m_filterThread->wait(1000); // Chờ tối đa 1 giây
    }

    // 1. Yêu cầu UI xóa sạch (việc này nhanh)
    emit clearPacketTable();

    // 2. Tạo một luồng (thread) "dùng một lần"
    m_filterThread = QThread::create([this]() {

        QList<PacketData> filteredList; // Danh sách kết quả

        // 3. Lặp lại danh sách chính (trên LUỒNG NỀN)
        for (const PacketData &packet : m_allPackets) {

            // (Kiểm tra xem luồng chính có yêu cầu dừng không)
            if (QThread::currentThread()->isInterruptionRequested()) {
                return; // Dừng việc lọc
            }

            // 4. Chỉ thêm lại những gói tin khớp
            if (m_filterEngine->packetMatches(packet)) {
                filteredList.append(packet);
            }
        }

        // 5. Gửi TOÀN BỘ kết quả đã lọc lên luồng chính
        if (!filteredList.isEmpty()) {
            QMetaObject::invokeMethod(this, [this, filteredList](){
                emit displayFilteredPackets(filteredList);
            }, Qt::QueuedConnection);
        }
    });

    // 6. Tự động xóa luồng khi nó chạy xong
    connect(m_filterThread, &QThread::finished, m_filterThread, &QObject::deleteLater);

    // 7. Bắt đầu luồng lọc
    m_filterThread->start();
}

void AppController::loadInterfaces()
{
    auto devices = InterfaceManager::getDevices();
    m_mainWindow->setDevices(devices);
}
