#include "AppController.hpp"
#include "../Core/Capture/InterfaceManager.hpp"
#include <QDebug>
#include <QDateTime>
#include <QFileDialog>     // <-- THÊM MỚI
#include <QMessageBox>     // <-- THÊM MỚI
#include <QDir>            // <-- THÊM MỚI
#include <QCoreApplication> // <-- THÊM MỚI
#include <pcap.h>          // <-- THÊM MỚI (Cần cho việc Save)

AppController::AppController(MainWindow *mainWindow, QObject *parent)
    : QObject(parent),
    m_mainWindow(mainWindow)
{
    // --- Khởi tạo Core ---
    m_captureEngine = new CaptureEngine(this);
    m_filterEngine = new DisplayFilterEngine(this);
    loadInterfaces();

    // --- Connect signal từ MainWindow (forward từ Page) ---
    connect(m_mainWindow, &MainWindow::interfaceSelected, this, &AppController::onInterfaceSelected);
    connect(m_mainWindow, &MainWindow::openFileRequested, this, &AppController::onOpenFileRequested);

    // --- THÊM MỚI: Kết nối tín hiệu Save ---
    // (Giả sử MainWindow cũng forward tín hiệu 'saveFileRequested'
    //  giống như 'openFileRequested' từ FileMenu)
    connect(m_mainWindow, &MainWindow::saveFileRequested, // <-- TÊN SIGNAL MỚI (từ FileMenu)
            this, &AppController::onSaveFileRequested);

    connect(m_mainWindow, &MainWindow::onRestartCaptureClicked, this, &AppController::onRestartCaptureClicked);
    connect(m_mainWindow, &MainWindow::onStopCaptureClicked, this, &AppController::onStopCaptureClicked);
    connect(m_mainWindow, &MainWindow::onPauseCaptureClicked, this, &AppController::onPauseCaptureClicked);
    connect(m_mainWindow, &MainWindow::onApplyFilterClicked, this, &AppController::onApplyFilterClicked);

    // --- Connect signal từ Core ---
    connect(m_captureEngine, &CaptureEngine::packetCaptured, this, &AppController::onPacketCaptured);

    // --- Connect TÍN HIỆU (Signal) của AppController VỚI (Slot) của MainWindow ---
    connect(this, &AppController::displayNewPacket, m_mainWindow, &MainWindow::addPacketToTable);
    connect(this, &AppController::clearPacketTable, m_mainWindow, &MainWindow::clearPacketTable);
    connect(this, &AppController::displayFilterError, m_mainWindow, &MainWindow::showFilterError);
}

void AppController::onInterfaceSelected(const QString &interfaceName, const QString &filterText)
{
    qDebug() << "Interface selected:" << interfaceName << "Filter:" << filterText;

    m_allPackets.clear();
    m_filterEngine->setFilter("");
    emit clearPacketTable();

    m_captureEngine->setInterface(interfaceName);
    m_captureEngine->setCaptureFilter(filterText);
    m_captureEngine->startCapture();

    m_mainWindow->showCapturePage();
}

/**
 * @brief (ĐÃ CẬP NHẬT) Xử lý khi người dùng chọn "Open File"
 */
void AppController::onOpenFileRequested()
{
    qDebug() << "Open file requested";

    m_captureEngine->stopCapture();

    QString defaultDir = QCoreApplication::applicationDirPath() + "/FileSave";
    QDir dir(defaultDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString filePath = QFileDialog::getOpenFileName(
        m_mainWindow,
        tr("Open Pcap File"),
        defaultDir,
        tr("Pcap Files (*.pcap *.pcapng)")
        );

    if (filePath.isEmpty()) {
        return;
    }

    m_allPackets.clear();
    m_filterEngine->setFilter("");
    emit clearPacketTable();

    // --- ĐÃ SỬA: BỎ COMMENT DÒNG NÀY ---
    m_captureEngine->startCaptureFromFile(filePath);
    // --- KẾT THÚC SỬA ---

    m_mainWindow->showCapturePage();
}

/**
 * @brief (ĐÃ CẬP NHẬT) Lưu vào file tại thư mục mặc định
 */
void AppController::onSaveFileRequested()
{
    qDebug() << "Save file requested";

    // 1. Kiểm tra xem có gì để lưu không
    if (m_allPackets.isEmpty()) {
        QMessageBox::warning(m_mainWindow, "Save Error", "There are no packets to save.");
        return;
    }

    // 2. --- ĐÃ THAY ĐỔI: Tạo đường dẫn mặc định ---
    QString defaultDir = QCoreApplication::applicationDirPath() + "/FileSave";
    QDir dir(defaultDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 3. Mở hộp thoại lưu file tại đường dẫn đó
    QString filePath = QFileDialog::getSaveFileName(
        m_mainWindow,
        tr("Save File As..."),
        defaultDir + "/capture.pcap", // <-- Đường dẫn + tên file mặc định
        tr("Pcap Files (*.pcap)")
        );
    // --- KẾT THÚC THAY ĐỔI ---

    if (filePath.isEmpty()) {
        return; // Người dùng nhấn Cancel
    }

    // 3. Mở pcap dumper (bộ ghi)
    pcap_t *pcap_handle = pcap_open_dead(DLT_EN10MB, 65535);
    if (!pcap_handle) {
        QMessageBox::critical(m_mainWindow, "Save Error", "Could not create pcap handle for writing.");
        return;
    }

    pcap_dumper_t *dumper = pcap_dump_open(pcap_handle, filePath.toStdString().c_str());
    if (!dumper) {
        QMessageBox::critical(m_mainWindow, "Save Error", QString("Could not open file for writing: %1").arg(pcap_geterr(pcap_handle)));
        pcap_close(pcap_handle);
        return;
    }

    // 4. Lặp qua "danh sách chính" và ghi (dump) từng gói tin
    for (const PacketData &packet : m_allPackets) {
        pcap_pkthdr header;

        header.ts.tv_sec = packet.timestamp.tv_sec;
        header.ts.tv_usec = packet.timestamp.tv_nsec / 1000;
        header.caplen = packet.cap_length;
        header.len = packet.wire_length;

        pcap_dump(
            reinterpret_cast<u_char*>(dumper),
            &header,
            packet.raw_packet.data()
            );
    }

    // 5. Đóng file
    pcap_dump_close(dumper);
    pcap_close(pcap_handle);

    QMessageBox::information(m_mainWindow, "Save Successful",
                             QString("Successfully saved %1 packets to %2")
                                 .arg(m_allPackets.size())
                                 .arg(filePath));
}


void AppController::onRestartCaptureClicked()
{
    qDebug() << "Restart capture";

    m_allPackets.clear();
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

    refreshFullDisplay();
}

/**
 * @brief (Đang dùng phiên bản 1-1, sẽ gây lag)
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
 * @brief (Đang dùng phiên bản 1-1, sẽ gây lag)
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
