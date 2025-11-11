#pragma once
#include <QObject>
#include <QList>
#include "../UI/MainWindow.hpp"
#include "../Core/Capture/CaptureEngine.hpp"
#include "../Common/PacketData.hpp"
#include "../Controller/ControllerLib/DisplayFilterEngine.hpp"

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(MainWindow *mainWindow, QObject *parent = nullptr);

signals:
    void displayNewPacket(const PacketData &packet);
    void clearPacketTable();

    /**
     * @brief (THÊM MỚI) Gửi lỗi cú pháp filter lên UI
     */
    void displayFilterError(const QString &errorText);

private:
    MainWindow *m_mainWindow;
    CaptureEngine *m_captureEngine;
    void loadInterfaces();
    void refreshFullDisplay();

    // --- BIẾN THÀNH VIÊN ĐÃ THAY ĐỔI ---
    QList<PacketData> m_allPackets;
    // QString m_currentDisplayFilter;
    DisplayFilterEngine *m_filterEngine;

private slots:
    void onInterfaceSelected(const QString &interfaceName, const QString &filterText);
    void onOpenFileRequested();
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();
    void onApplyFilterClicked(const QString &filterText);
    void onPacketCaptured(const PacketData &packet);
};
