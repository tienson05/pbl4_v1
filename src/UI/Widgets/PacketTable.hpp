#ifndef PACKETTABLE_HPP
#define PACKETTABLE_HPP

#include <QWidget>
#include <QList>        // (Cần cho QList)
#include <QTimer>       // (Cần cho QTimer)
#include <QRegularExpression>
#include <QTreeWidget>
#include "../../Common/PacketData.hpp" // (Cần cho PacketData)

// Khai báo trước (Forward declarations)
class QTableWidget;
class QTextEdit;
class QTableWidgetItem;

class PacketTable : public QWidget
{
    Q_OBJECT

public:
    explicit PacketTable(QWidget *parent = nullptr);

public slots:
    /**
     * @brief (ĐÃ THAY ĐỔI) Slot nhận MỘT gói tin (từ AppController cũ)
     * Nó chỉ thêm vào bộ đệm (buffer).
     */
    void onPacketReceived(const PacketData &packet);

    /**
     * @brief (ĐÃ THAY ĐỔI) Slot nhận MỘT LÔ (batch) gói tin
     * Dùng con trỏ để nhận dữ liệu siêu nhanh (O(1)).
     */
    void onPacketsReceived(QList<PacketData>* packets);

    /**
     * @brief (Giữ nguyên) Xóa sạch dữ liệu.
     */
    void clearData();

private slots:
    /**
     * @brief (MỚI) Được gọi bởi QTimer, xử lý một phần nhỏ của buffer
     */
    void processPacketChunk();

    // (Giữ nguyên)
    void onPacketRowSelected(QTableWidgetItem *item);

private:
    /**
     * @brief (MỚI) Hàm helper (nội bộ) để chèn 1 hàng vào bảng.
     */
    void insertPacketRow(const PacketData& packet);

    // (Giữ nguyên)
    void setupUI();
    void showPacketDetails(const PacketData &packet);
    void showHexDump(const PacketData &packet);
    void addField(QTreeWidgetItem *parent, const QString &name, const QString &value);

public: // (Các hàm static giữ nguyên)
    static bool packetMatchesFilter(const PacketData &packet, const QString &filterText);
    static QString getSourceAddress(const PacketData &p);
    static QString getDestinationAddress(const PacketData &p);
    static QString getProtocolName(const PacketData &p);
    static QString getInfo(const PacketData &p);

private: // (Các hàm static giữ nguyên)
    static QString macToString(const std::array<uint8_t, 6>& mac);
    static QString ipToString(uint32_t ip);
    static QString ipv6ToString(const std::array<uint8_t, 16>& ip);
    static QString getEtherTypeName(uint16_t type);
    static QString getIpProtoName(uint8_t proto);

    // Thành viên UI
    QTableWidget *packetList;
    QTreeWidget *packetDetails;
    QTextEdit *packetBytes;

    // --- (THÊM MỚI) Thành viên cho logic "live" ---
    QList<PacketData> m_packetBuffer; // Hàng đợi
    QTimer* m_updateTimer;          // Đồng hồ
    bool m_isUserAtBottom = true;
};

#endif // PACKETTABLE_HPP
