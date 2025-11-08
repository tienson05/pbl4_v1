#pragma once
#include <QObject>
#include "../UI/MainWindow.hpp"
#include "../Core/Capture/CaptureEngine.hpp"
#include "../Common/PacketData.hpp"

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(MainWindow *mainWindow, QObject *parent = nullptr);

private:
    MainWindow *m_mainWindow;
    CaptureEngine *m_captureEngine;
    void loadInterfaces();

private slots:
    // --- Xử lý signal từ MainWindow / Page ---
    void onInterfaceSelected(const QString &interfaceName, const QString &filterText);
    void onOpenFileRequested();
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();
    void onApplyFilterClicked(const QString &filterText);
    void onPacketCaptured(const PacketData &packet);
};
