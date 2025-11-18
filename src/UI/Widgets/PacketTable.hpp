#ifndef PACKETTABLE_HPP
#define PACKETTABLE_HPP

#include <QWidget>
#include <QList>
#include <QTimer>
#include <QRegularExpression>
#include <QTreeWidget>
#include "../../Common/PacketData.hpp"
#include "../../Controller/ControllerLib/DisplayFilterEngine.hpp" // <-- (MỚI) Include Engine lọc

// Khai báo trước (Forward declarations)
class QTableWidget;
class QTextEdit;
class QTableWidgetItem;
class QTreeWidgetItem;

class PacketTable : public QWidget
{
    Q_OBJECT

public:
    explicit PacketTable(QWidget *parent = nullptr);

public slots:
    // Nhận gói tin từ Capture Engine
    void onPacketReceived(const PacketData &packet);
    void onPacketsReceived(QList<PacketData>* packets);

    // Xóa dữ liệu
    void clearData();

    // (MỚI) Slot nhận tín hiệu lọc từ thanh Header
    void applyFilter(const QString& filterText);

private slots:
    // Xử lý bộ đệm (Anti-lag)
    void processPacketChunk();

    // Sự kiện Click UI
    void onPacketRowSelected(QTableWidgetItem *item);
    void onDetailRowSelected(QTreeWidgetItem *item, int column);

private:
    // (MỚI) Hàm vẽ lại bảng khi thay đổi Filter (ẩn/hiện các dòng)
    void refreshTable();

    // Hàm vẽ 1 dòng lên bảng (chỉ dùng nội bộ khi thỏa mãn filter)
    void insertPacketRow(const PacketData& packet);

    void setupUI();

    // Hiển thị chi tiết và Hex dump
    void showPacketDetails(const PacketData &packet);
    void showHexDump(const PacketData &packet, int highlight_offset = -1, int highlight_len = 0);
    void addField(QTreeWidgetItem *parent, const QString &name, const QString &value, int offset = -1, int length = 0);


public: // (Các hàm static hỗ trợ format chuỗi)
    static QString getSourceAddress(const PacketData &p);
    static QString getDestinationAddress(const PacketData &p);
    static QString getProtocolName(const PacketData &p);
    static QString getInfo(const PacketData &p);

    // Helper nội bộ của class (static)
    static QString macToString(const std::array<uint8_t, 6>& mac);
    static QString ipToString(uint32_t ip);
    static QString ipv6ToString(const std::array<uint8_t, 16>& ip);
    static QString getEtherTypeName(uint16_t type);
    static QString getIpProtoName(uint8_t proto);

private:
    // --- THÀNH VIÊN UI ---
    QTableWidget *packetList;
    QTreeWidget *packetDetails;
    QTextEdit *packetBytes;

    // Lưu gói tin đang được chọn để hiển thị Hex/Details
    PacketData m_currentSelectedPacket;

    // --- THÀNH VIÊN XỬ LÝ BUFFER (CHỐNG LAG) ---
    QList<PacketData> m_packetBuffer; // Hàng đợi chờ xử lý
    QTimer* m_updateTimer;
    bool m_isUserAtBottom = true;

    // --- (MỚI) THÀNH VIÊN CHO DISPLAY FILTER ---
    QList<PacketData> m_allPackets;      // Kho chứa TOÀN BỘ gói tin (không bị mất khi lọc)
    QString m_currentFilter;             // Từ khóa lọc hiện tại (ví dụ: "http")
    DisplayFilterEngine m_filterEngine;  // Engine xử lý logic lọc
};

#endif // PACKETTABLE_HPP
