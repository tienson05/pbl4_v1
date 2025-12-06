#ifndef PACKETTABLE_HPP
#define PACKETTABLE_HPP

#include <QWidget>
#include <QList>
#include <QTimer>
#include <QTreeWidget>
#include "../../Common/PacketData.hpp"
#include "../../Controller/ControllerLib/DisplayFilterEngine.hpp"

// Forward declarations
class QTableWidget;
class QTextEdit;
class QTableWidgetItem;
class QTreeWidgetItem;

class PacketTable : public QWidget
{
    Q_OBJECT

public:
    explicit PacketTable(QWidget *parent = nullptr);

signals:
    // Bắn tín hiệu khi chọn "Follow Stream"
    void filterRequested(const QString &filterText);

public slots:
    // Nhận dữ liệu
    void onPacketReceived(const PacketData &packet);
    void onPacketsReceived(QList<PacketData>* packets);

    // Xử lý dữ liệu
    void clearData();
    void applyFilter(const QString& filterText);

private slots:
    // Slot nội bộ
    void processPacketChunk();
    void onPacketRowSelected(QTableWidgetItem *item);
    void onDetailRowSelected(QTreeWidgetItem *item, int column);

private:
    // UI Setup & Logic hiển thị bảng
    void setupUI();
    void showContextMenu(const QPoint &pos);
    void refreshTable();
    void insertPacketRow(const PacketData& packet);


private:
    // --- UI COMPONENTS ---
    QTableWidget *packetList;
    QTreeWidget *packetDetails;
    QTextEdit *packetBytes;

    // --- DATA STATE ---
    PacketData m_currentSelectedPacket;

    // --- BUFFER & TIMER (Anti-lag) ---
    QList<PacketData> m_packetBuffer;
    QTimer* m_updateTimer;
    bool m_isUserAtBottom = true;

    // --- DISPLAY FILTER DATA ---
    QList<PacketData> m_allPackets;      // Kho lưu trữ gốc
    QString m_currentFilter;             // Filter text hiện tại
    DisplayFilterEngine m_filterEngine;  // Engine lọc
};

#endif // PACKETTABLE_HPP
