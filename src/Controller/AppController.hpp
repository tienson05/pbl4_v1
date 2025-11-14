#pragma once
#include <QObject>
#include <QList>
#include "../UI/MainWindow.hpp"
#include "../Core/Capture/CaptureEngine.hpp"
#include "../Common/PacketData.hpp"
#include "../Controller/ControllerLib/DisplayFilterEngine.hpp"
#include <QThread> // <-- THÊM MỚI

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(MainWindow *mainWindow, QObject *parent = nullptr);

signals:
    void displayNewPacket(const PacketData &packet);
    void clearPacketTable();
    void displayFilterError(const QString &errorText);

    /**
     * @brief (THÊM MỚI) Tín hiệu này dùng để gửi một LÔ
     * gói tin đã được lọc lên UI, thay vì gửi từng cái.
     */
    void displayFilteredPackets(const QList<PacketData> &packets);

private:
    MainWindow *m_mainWindow;
    CaptureEngine *m_captureEngine;
    void loadInterfaces();
    void refreshFullDisplay();

    QList<PacketData> m_allPackets;
    DisplayFilterEngine *m_filterEngine;

    QThread* m_filterThread = nullptr; // <-- THÊM MỚI: Luồng để lọc

private slots:
    // (Các slot từ UI giữ nguyên)
    void onInterfaceSelected(const QString &interfaceName, const QString &filterText);
    void onOpenFileRequested();
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();
    void onApplyFilterClicked(const QString &filterText);

    // Slot nhận lô gói tin từ Core
    void onPacketsCaptured(const QList<PacketData> &packets);
};
