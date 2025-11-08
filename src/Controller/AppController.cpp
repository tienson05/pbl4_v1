#include "AppController.hpp"
#include "../Core/Capture/InterfaceManager.hpp"
#include <QDebug>
#include <QDateTime>
AppController::AppController(MainWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    // --- Khởi tạo Core ---
    m_captureEngine = new CaptureEngine(this);
    loadInterfaces();

    // --- Connect signal từ MainWindow (forward từ Page) ---
    connect(m_mainWindow, &MainWindow::interfaceSelected,
            this, &AppController::onInterfaceSelected);

    connect(m_mainWindow, &MainWindow::openFileRequested,
            this, &AppController::onOpenFileRequested);

    connect(m_mainWindow, &MainWindow::onRestartCaptureClicked,
            this, &AppController::onRestartCaptureClicked);

    connect(m_mainWindow, &MainWindow::onStopCaptureClicked,
            this, &AppController::onStopCaptureClicked);

    connect(m_mainWindow, &MainWindow::onPauseCaptureClicked,
            this, &AppController::onPauseCaptureClicked);

    connect(m_mainWindow, &MainWindow::onApplyFilterClicked,
            this, &AppController::onApplyFilterClicked);

    connect(m_captureEngine, &CaptureEngine::packetCaptured,
            this, &AppController::onPacketCaptured);
}

void AppController::onInterfaceSelected(const QString &interfaceName, const QString &filterText)
{
    qDebug() << "Interface selected:" << interfaceName << "Filter:" << filterText;
    m_captureEngine->setInterface(interfaceName);
    m_captureEngine->setCaptureFilter(filterText);
    m_captureEngine->startCapture();
    // Chuyển sang trang CapturePage
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
    qDebug() << "Apply filter:" << filterText;
    m_captureEngine->setDisplayFilter(filterText);
}

void AppController::onPacketCaptured(const PacketData &packet)
{
    // === Gửi đến MainWindow ===
    QMetaObject::invokeMethod(m_mainWindow, [this, packet]() {
        m_mainWindow->addPacketToTable(packet);
    }, Qt::QueuedConnection);
}

void AppController::loadInterfaces()
{
    auto devices = InterfaceManager::getDevices();
    m_mainWindow->setDevices(devices);
}

