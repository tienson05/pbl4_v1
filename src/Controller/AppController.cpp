#include "AppController.hpp"
#include "../Core/Capture/InterfaceManager.hpp"
#include "../UI/Widgets/StatisticsDialog.hpp" // <-- Include Dialog
#include <QDebug>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QCoreApplication>
#include <pcap.h>

AppController::AppController(MainWindow *mainWindow, QObject *parent)
    : QObject(parent),
    m_mainWindow(mainWindow),
    m_captureEngine(nullptr),
    m_filterEngine(nullptr),
    m_statsManager(nullptr),
    m_statisticsDialog(nullptr) // <-- 1. KHỞI TẠO LÀ NULLPTR
{
    // --- Khởi tạo Core ---
    m_captureEngine = new CaptureEngine(this);
    m_filterEngine = new DisplayFilterEngine(this);

    // --- 2. KHỞI TẠO BỘ ĐẾM THỐNG KÊ ---
    m_statsManager = new StatisticsManager(this);

    loadInterfaces();

    // --- Connect signal từ MainWindow (forward từ Page) ---
    connect(m_mainWindow, &MainWindow::interfaceSelected, this, &AppController::onInterfaceSelected);
    connect(m_mainWindow, &MainWindow::openFileRequested, this, &AppController::onOpenFileRequested);
    connect(m_mainWindow, &MainWindow::saveFileRequested, this, &AppController::onSaveFileRequested);
    connect(m_mainWindow, &MainWindow::onRestartCaptureClicked, this, &AppController::onRestartCaptureClicked);
    connect(m_mainWindow, &MainWindow::onStopCaptureClicked, this, &AppController::onStopCaptureClicked);
    connect(m_mainWindow, &MainWindow::onPauseCaptureClicked, this, &AppController::onPauseCaptureClicked);
    connect(m_mainWindow, &MainWindow::onApplyFilterClicked, this, &AppController::onApplyFilterClicked);

    // --- 3. KẾT NỐI MENU THỐNG KÊ (Dùng tên signal đúng) ---
    // (Kết nối với signal 'analyzeStatisticsRequested' của MainWindow)
    connect(m_mainWindow, &MainWindow::analyzeStatisticsRequested,
            this, &AppController::onStatisticsMenuClicked);

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
    m_statsManager->clear(); // <-- 4. RESET BỘ ĐẾM
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
    m_statsManager->clear(); // <-- 4. RESET BỘ ĐẾM
    m_filterEngine->setFilter("");
    emit clearPacketTable();
    m_captureEngine->startCaptureFromFile(filePath);
    m_mainWindow->showCapturePage();
}

void AppController::onSaveFileRequested()
{
    qDebug() << "Save file requested";
    if (m_allPackets.isEmpty()) {
        QMessageBox::warning(m_mainWindow, "Save Error", "There are no packets to save.");
        return;
    }
    QString defaultDir = QCoreApplication::applicationDirPath() + "/FileSave";
    QDir dir(defaultDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString filePath = QFileDialog::getSaveFileName(
        m_mainWindow,
        tr("Save File As..."),
        defaultDir + "/capture.pcap",
        tr("Pcap Files (*.pcap)")
        );
    if (filePath.isEmpty()) {
        return;
    }
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
    m_statsManager->clear(); // <-- 4. RESET BỘ ĐẾM
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

void AppController::onPacketCaptured(const PacketData &packet)
{
    // 1. Luôn luôn lưu vào danh sách chính
    m_allPackets.append(packet);
    // 2. Gửi gói tin cho bộ đếm (ĐẾM LIÊN TỤC)
    m_statsManager->processPacket(packet);
    // 3. Chỉ gửi tín hiệu hiển thị nếu nó khớp
    if (m_filterEngine->packetMatches(packet))
    {
        emit displayNewPacket(packet);
    }
}

void AppController::refreshFullDisplay()
{
    // (CẢNH BÁO: VÒNG LẶP NÀY SẼ GÂY TREO APP)
    emit clearPacketTable();
    for (const PacketData &packet : m_allPackets) {
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

// --- (HÀM ĐÃ ĐƯỢC CẬP NHẬT LÊN "LIVE") ---
void AppController::onStatisticsMenuClicked()
{
    qDebug() << "Statistics requested";

    // 1. Nếu cửa sổ CHƯA TỒN TẠI (hoặc đã bị đóng)
    if (!m_statisticsDialog)
    {
        // 2. Tạo nó MỘT LẦN, và truyền manager vào
        m_statisticsDialog = new StatisticsDialog(m_statsManager, m_mainWindow);

        // 3. Kết nối tín hiệu 'destroyed' để set con trỏ về null
        connect(m_statisticsDialog, &QObject::destroyed, this, [this](){
            m_statisticsDialog = nullptr;
        });
    }

    // 4. HIỂN THỊ cửa sổ (dùng show() thay vì exec())
    m_statisticsDialog->show();
    m_statisticsDialog->activateWindow(); // Đưa nó lên trên
    m_statisticsDialog->raise();
}
