#pragma once
#include <QObject>
#include <QList>
#include <QMutex> // <-- 1. THÊM MỚI (Để bảo vệ luồng)
#include "../UI/MainWindow.hpp"
#include "../Core/Capture/CaptureEngine.hpp"
#include "../Common/PacketData.hpp"
#include "../Controller/ControllerLib/DisplayFilterEngine.hpp"
#include "StatisticsManager.hpp"
#include <QtConcurrent/QtConcurrent> // (Cần cho đa luồng)
#include "ControllerLib/ConversationManager.hpp"

class StatisticsDialog; // Khai báo trước

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(MainWindow *mainWindow, QObject *parent = nullptr);

signals:
    /**
     * @brief (ĐÃ THAY ĐỔI) Gửi một "lô" (batch) gói tin đã lọc
     * đến PacketTable để hiển thị.
     */
    void displayNewPackets(QList<PacketData>* packetBatch);

    // (Tín hiệu này vẫn giữ nguyên)
    void clearPacketTable();
    void displayFilterError(const QString &errorText);

private:
    MainWindow *m_mainWindow;
    CaptureEngine *m_captureEngine;
    void loadInterfaces();
    void refreshFullDisplay();

    QList<PacketData> m_allPackets;
    DisplayFilterEngine *m_filterEngine;
    StatisticsManager* m_statsManager;
    StatisticsDialog* m_statisticsDialog;
    ConversationManager* m_convManager; // "Bộ não" stateful mới

    // --- 2. THÊM MỚI ---
    QMutex m_allPacketsMutex; // Ổ khóa để bảo vệ m_allPackets

private slots:
    // Slots nhận từ UI
    void onInterfaceSelected(const QString &interfaceName, const QString &filterText);
    void onOpenFileRequested();
    void onSaveFileRequested();
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();
    void onApplyFilterClicked(const QString &filterText);
    void onStatisticsMenuClicked();

    /**
     * @brief (ĐÃ THAY ĐỔI) Slot nhận "lô" gói tin từ CaptureEngine
     */
    void onPacketsCaptured(QList<PacketData>* packetBatch);

    /**
     * @brief (MỚI) Slot nhận kết quả từ luồng lọc (filter) nền
     */
    void onFilteringFinished(QList<PacketData>* filteredPackets);
};
