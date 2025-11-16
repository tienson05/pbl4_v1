#include "AppController.hpp"
#include "../Core/Capture/InterfaceManager.hpp"
#include "../UI/Widgets/StatisticsDialog.hpp"
#include <QDebug>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QCoreApplication>
#include <pcap.h>
#include <QtConcurrent/QtConcurrent> // <-- THÊM MỚI (Cho luồng nền)

AppController::AppController(MainWindow *mainWindow, QObject *parent)
    : QObject(parent),
    m_mainWindow(mainWindow),
    m_captureEngine(nullptr),
    m_filterEngine(nullptr),
    m_statsManager(nullptr),
    m_statisticsDialog(nullptr),
    m_convManager(nullptr)

{
    // --- Khởi tạo Core ---
    m_captureEngine = new CaptureEngine(this);
    m_filterEngine = new DisplayFilterEngine(this);
    m_statsManager = new StatisticsManager(this);
    m_convManager = new ConversationManager(this);
    loadInterfaces();

    // --- (Các connect từ UI giữ nguyên) ---
    connect(m_mainWindow, &MainWindow::interfaceSelected, this, &AppController::onInterfaceSelected);
    connect(m_mainWindow, &MainWindow::openFileRequested, this, &AppController::onOpenFileRequested);
    connect(m_mainWindow, &MainWindow::saveFileRequested, this, &AppController::onSaveFileRequested);
    connect(m_mainWindow, &MainWindow::onRestartCaptureClicked, this, &AppController::onRestartCaptureClicked);
    connect(m_mainWindow, &MainWindow::onStopCaptureClicked, this, &AppController::onStopCaptureClicked);
    connect(m_mainWindow, &MainWindow::onPauseCaptureClicked, this, &AppController::onPauseCaptureClicked);
    connect(m_mainWindow, &MainWindow::onApplyFilterClicked, this, &AppController::onApplyFilterClicked);
    connect(m_mainWindow, &MainWindow::analyzeStatisticsRequested,
            this, &AppController::onStatisticsMenuClicked);

    // --- (SỬA ĐỔI) Connect signal từ Core (LÔ) ---
    connect(m_captureEngine, &CaptureEngine::packetsCaptured, // <-- Tín hiệu LÔ
            this, &AppController::onPacketsCaptured);        // <-- Slot LÔ

    // --- (SỬA ĐỔI) Connect TÍN HIỆU (Signal) của AppController VỚI (Slot) của MainWindow ---
    connect(this, &AppController::displayNewPackets,      // <-- Tín hiệu LÔ
            m_mainWindow, &MainWindow::addPacketsToTable); // <-- Slot LÔ
    connect(this, &AppController::clearPacketTable, m_mainWindow, &MainWindow::clearPacketTable);
    connect(this, &AppController::displayFilterError, m_mainWindow, &MainWindow::showFilterError);

    // (MỚI) Connect tín hiệu khi lọc xong
    connect(this, &AppController::onFilteringFinished,
            m_mainWindow, &MainWindow::addPacketsToTable);
}

void AppController::onInterfaceSelected(const QString &interfaceName, const QString &filterText)
{
    qDebug() << "Interface selected:" << interfaceName << "Filter:" << filterText;

    { // <-- (SỬA) Khóa (lock) m_allPackets
        QMutexLocker locker(&m_allPacketsMutex);
        m_allPackets.clear();
    }

    m_statsManager->clear();
    m_convManager->clear();
    m_filterEngine->setFilter("");
    emit clearPacketTable();

    m_captureEngine->setInterface(interfaceName);
    m_captureEngine->setCaptureFilter(filterText);
    m_captureEngine->startCapture();

    m_mainWindow->showCapturePage();
}

void AppController::onOpenFileRequested()
{
    qDebug() << "Open file requested";
    m_captureEngine->stopCapture();
    // ... (Code QFileDialog của bạn giữ nguyên) ...
    QString filePath = QFileDialog::getOpenFileName(m_mainWindow, tr("Open Pcap File"));
    if (filePath.isEmpty()) {
        return;
    }

    { // <-- (SỬA) Khóa (lock) m_allPackets
        QMutexLocker locker(&m_allPacketsMutex);
        m_allPackets.clear();
    }
    m_statsManager->clear();
    m_convManager->clear();
    m_filterEngine->setFilter("");
    emit clearPacketTable();
    m_captureEngine->startCaptureFromFile(filePath);
    m_mainWindow->showCapturePage();
}

void AppController::onSaveFileRequested()
{
    qDebug() << "Save file requested";

    // (THÊM MỚI) Tạo một bản copy an toàn để lưu
    QList<PacketData> packetsToSave;
    {
        QMutexLocker locker(&m_allPacketsMutex);
        packetsToSave = m_allPackets; // (Copy)
    }

    if (packetsToSave.isEmpty()) {
        QMessageBox::warning(m_mainWindow, "Save Error", "There are no packets to save.");
        return;
    }

    // ... (Code QFileDialog của bạn giữ nguyên) ...
    QString filePath = QFileDialog::getSaveFileName(m_mainWindow, tr("Save File As..."));
    if (filePath.isEmpty()) {
        return;
    }

    // ... (Code pcap_dumper của bạn giữ nguyên,
    //      NHƯNG lặp 'packetsToSave' thay vì 'm_allPackets')
    pcap_t *pcap_handle = pcap_open_dead(DLT_EN10MB, 65535);
    // ... (Kiểm tra lỗi) ...
    pcap_dumper_t *dumper = pcap_dump_open(pcap_handle, filePath.toStdString().c_str());
    // ... (Kiểm tra lỗi) ...

    for (const PacketData &packet : packetsToSave) { // <-- SỬA: Dùng 'packetsToSave'
        pcap_pkthdr header;
        // ... (code pcap_dump của bạn) ...
        header.ts.tv_sec = packet.timestamp.tv_sec;
        header.ts.tv_usec = packet.timestamp.tv_nsec / 1000;
        header.caplen = packet.cap_length;
        header.len = packet.wire_length;
        pcap_dump(reinterpret_cast<u_char*>(dumper), &header, packet.raw_packet.data());
    }

    pcap_dump_close(dumper);
    pcap_close(pcap_handle);
    QMessageBox::information(m_mainWindow, "Save Successful", "Save complete.");
}


void AppController::onRestartCaptureClicked()
{
    qDebug() << "Restart capture";

    { // <-- (SỬA) Khóa (lock) m_allPackets
        QMutexLocker locker(&m_allPacketsMutex);
        m_allPackets.clear();
    }

    m_statsManager->clear();
    m_convManager->clear();
    m_filterEngine->setFilter("");
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
    // (Hàm này giữ nguyên)
    qDebug() << "Pause/Resume capture";
    if (m_captureEngine->isPaused()) {
        m_captureEngine->resumeCapture();
    } else {
        m_captureEngine->pauseCapture();
    }
}

void AppController::onApplyFilterClicked(const QString &filterText)
{
    qDebug() << "Apply BPF display filter:" << filterText;
    if (!m_filterEngine->setFilter(filterText))
    {
        emit displayFilterError(m_filterEngine->getLastError());
        m_filterEngine->setFilter("");
    }

    // (SỬA) Gọi hàm refresh đa luồng mới
    refreshFullDisplay();
}

/**
 * @brief (ĐÃ SỬA) Xử lý LÔ gói tin từ luồng bắt (CaptureEngine)
 */
void AppController::onPacketsCaptured(QList<PacketData>* packetBatch)
{
    // (Hàm này giờ chạy trên LUỒNG CHÍNH,
    // vì CaptureEngine đã dùng QueuedConnection)
    // --- GỌI BỘ NÃO MỚI (TRƯỚC) ---
    // Lặp qua lô (batch) và gọi bộ não "stateful"
    // để nó SỬA ĐỔI (modify) các gói tin (ví dụ: gán nhãn "QUIC")
    for (PacketData &packet : *packetBatch) { // <-- Lặp bằng tham chiếu (reference)
        m_convManager->processPacket(packet);
    }

    // 1. (SỬA) Khóa và thêm lô vào danh sách chính
    {
        QMutexLocker locker(&m_allPacketsMutex);
        m_allPackets.append(*packetBatch);
    }

    // 2. Gửi lô cho bộ đếm (rất nhanh, chỉ lặp 50-100 gói)
    m_statsManager->processPackets(*packetBatch);

    // 3. (MỚI) Lọc lô này để hiển thị live
    QList<PacketData>* filteredBatch = new QList<PacketData>();
    for (const PacketData &packet : *packetBatch) {
        if (m_filterEngine->packetMatches(packet))
        {
            filteredBatch->append(packet);
        }
    }

    // 4. Xóa lô gốc (từ CaptureEngine)
    delete packetBatch;

    // 5. Gửi lô đã lọc (có thể rỗng) lên UI
    if (!filteredBatch->isEmpty()) {
        emit displayNewPackets(filteredBatch);
    } else {
        delete filteredBatch; // Xóa nếu rỗng
    }
}

/**
 * @brief (ĐÃ SỬA) Chạy lọc trên LUỒNG NỀN để không treo app
 */
void AppController::refreshFullDisplay()
{
    // 1. Yêu cầu UI xóa sạch (chạy trên luồng UI)
    emit clearPacketTable();

    // 2. (MỚI) Chạy tác vụ lọc (nặng) trên một luồng khác
    QtConcurrent::run([this]() {
        qDebug() << "Filter thread started...";

        QList<PacketData>* filteredList = new QList<PacketData>();

        // 3. (MỚI) Khóa m_allPackets CHỈ trong lúc đọc
        {
            QMutexLocker locker(&m_allPacketsMutex);

            // 4. Lặp qua danh sách chính (trên luồng nền)
            for (const PacketData &packet : m_allPackets) {
                if (m_filterEngine->packetMatches(packet)) {
                    filteredList->append(packet);
                }
            }
        } // (Mutex được tự động mở khóa ở đây)

        qDebug() << "Filter thread finished. Emitting" << filteredList->size() << "packets.";

        // 5. Gửi con trỏ chứa kết quả về luồng UI
        // (Chúng ta emit tín hiệu 'onFilteringFinished'
        //  đã được connect với 'addPacketsToTable' trong constructor)
        QMetaObject::invokeMethod(this, [this, filteredList](){
            emit onFilteringFinished(filteredList);
        }, Qt::QueuedConnection);
    });
}

/**
 * @brief (MỚI) Slot nhận kết quả từ luồng lọc
 */
void AppController::onFilteringFinished(QList<PacketData>* filteredPackets)
{
    // (Hàm này chạy trên luồng UI)
    // Nó chỉ phát tín hiệu đã được kết nối với PacketTable
    // (Chúng ta làm vậy để giữ AppController sạch sẽ)
    emit displayNewPackets(filteredPackets);
}


void AppController::loadInterfaces()
{
    auto devices = InterfaceManager::getDevices();
    m_mainWindow->setDevices(devices);
}

void AppController::onStatisticsMenuClicked()
{
    // (Hàm này giữ nguyên như code của bạn)
    qDebug() << "Statistics requested";
    if (!m_statisticsDialog)
    {
        m_statisticsDialog = new StatisticsDialog(m_statsManager, m_mainWindow);
        connect(m_statisticsDialog, &QObject::destroyed, this, [this](){
            m_statisticsDialog = nullptr;
        });
    }
    m_statisticsDialog->show();
    m_statisticsDialog->activateWindow();
    m_statisticsDialog->raise();
}
