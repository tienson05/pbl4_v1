#pragma once
#include <QObject>
#include <QList>
#include "../UI/MainWindow.hpp"
#include "../Core/Capture/CaptureEngine.hpp"
#include "../Common/PacketData.hpp"
#include "../Controller/ControllerLib/DisplayFilterEngine.hpp"
#include "StatisticsManager.hpp"

// (KHAI BÁO TRƯỚC)
class StatisticsDialog;

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(MainWindow *mainWindow, QObject *parent = nullptr);

signals:
    // (Tín hiệu để cập nhật bảng)
    void displayNewPacket(const PacketData &packet);
    void clearPacketTable();

    // (Tín hiệu báo lỗi filter)
    void displayFilterError(const QString &errorText);

private:
    MainWindow *m_mainWindow;
    CaptureEngine *m_captureEngine;
    void loadInterfaces();
    void refreshFullDisplay();

    QList<PacketData> m_allPackets;
    DisplayFilterEngine *m_filterEngine;
    StatisticsManager* m_statsManager;

    // (Lưu con trỏ đến dialog duy nhất)
    StatisticsDialog* m_statisticsDialog;

private slots:
    // Slots nhận từ UI
    void onInterfaceSelected(const QString &interfaceName, const QString &filterText);
    void onOpenFileRequested();
    void onSaveFileRequested();
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();
    void onApplyFilterClicked(const QString &filterText);
    void onStatisticsMenuClicked(); // Slot cho menu

    // Slot nhận từ Core
    void onPacketCaptured(const PacketData &packet);
};
