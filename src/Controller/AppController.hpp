#ifndef APPCONTROLLER_HPP
#define APPCONTROLLER_HPP

#include <QObject>
#include <QMutex>
#include "../UI/MainWindow.hpp"
#include "../Core/Capture/CaptureEngine.hpp"
#include "ControllerLib/DisplayFilterEngine.hpp"
#include "StatisticsManager.hpp"
#include "ControllerLib/ConversationManager.hpp"
#include "../Widgets/StatisticsDialog.hpp"
#include "../Widgets/IOGraphDialog.hpp"


class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(MainWindow *mainWindow, QObject *parent = nullptr);

public slots:
    // UI Actions
    void onInterfaceSelected(const QString &interfaceName, const QString &filterText);
    void onOpenFileRequested();
    void onSaveFileRequested();
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();

    // (QUAN TRỌNG) Slot này nhận chuỗi lọc
    void onApplyFilterClicked(const QString &filterText);
    void onStatisticsMenuClicked();
    void onIOGraphMenuClicked(); // <-- THÊM SLOT MỚI

    // Core Signals
    void onPacketsCaptured(QList<PacketData>* packetBatch);

    // Helper slot
    void onFilteringFinished(QList<PacketData>* filteredPackets);

signals:
    void displayNewPackets(QList<PacketData>* packets);
    void clearPacketTable();
    void displayFilterError(const QString &error);

private:
    void loadInterfaces();
    void refreshFullDisplay(); // Hàm chạy lọc lại toàn bộ

    MainWindow *m_mainWindow;
    CaptureEngine *m_captureEngine;
    DisplayFilterEngine *m_filterEngine;
    StatisticsManager *m_statsManager;
    StatisticsDialog *m_statisticsDialog;
    ConversationManager *m_convManager;
    IOGraphDialog *m_ioGraphDialog;


    // Dữ liệu
    QList<PacketData> m_allPackets;
    QMutex m_allPacketsMutex;

    //Lưu trữ từ khóa lọc hiện tại (ví dụ: "http")
    QString m_currentFilterText;
};

#endif // APPCONTROLLER_HPP
