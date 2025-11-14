#ifndef PACKETTABLE_HPP
#define PACKETTABLE_HPP

#include <QWidget>
#include <QList>              // <-- THÊM MỚI (Cần cho QList)
#include <QTimer>             // <-- THÊM MỚI (Cần cho QTimer)
#include <QRegularExpression>
#include <QTreeWidget> // <-- THÊM LẠI DÒNG NÀY
#include "../../Common/PacketData.hpp"

// Khai báo trước (Forward declarations) để tránh include header nặng
class QTableWidget;
class QTreeWidget;
class QTextEdit;
class QTableWidgetItem;

class PacketTable : public QWidget
{
    Q_OBJECT
public:
    explicit PacketTable(QWidget *parent = nullptr);

public slots:
    /**
     * @brief (SỬA) Slot nhận MỘT gói tin (ví dụ: bắt live).
     * Sẽ thêm gói tin vào hàng đợi (buffer).
     */
    void onPacketReceived(const PacketData &packet);

    /**
     * @brief (SỬA) Slot nhận MỘT LÔ gói tin (ví dụ: đọc file).
     * Sẽ thêm tất cả vào hàng đợi (buffer).
     */
    void onPacketsReceived(const QList<PacketData> &packets);

    /**
     * @brief Xóa sạch dữ liệu khỏi bảng và cây.
     */
    void clearData();

private slots:
    /**
     * @brief (MỚI) Slot này được gọi bởi m_updateTimer.
     * Xử lý một phần nhỏ của hàng đợi (buffer) để tránh lag.
     */
    void processPacketChunk();

    /**
     * @brief Slot khi người dùng click vào một hàng.
     */
    void onPacketRowSelected(QTableWidgetItem *item);

private:
    /**
     * @brief (MỚI) Hàm helper để chèn 1 hàng vào QTableWidget.
     * Được gọi bởi processPacketChunk() và onPacketReceived().
     */
    void insertPacketRow(const PacketData& packet);

    // Các hàm helper UI (Giữ nguyên)
    void setupUI();
    void showPacketDetails(const PacketData &packet);
    void showHexDump(const PacketData &packet);
    void addField(QTreeWidgetItem *parent, const QString &name, const QString &value);

public: // (Các hàm static helper của bạn - Giữ nguyên)
    static bool packetMatchesFilter(const PacketData &packet, const QString &filterText);
    static QString getSourceAddress(const PacketData &p);
    static QString getDestinationAddress(const PacketData &p);
    static QString getProtocolName(const PacketData &p);
    static QString getInfo(const PacketData &p);

private: // (Các hàm static helper của bạn - Giữ nguyên)
    static QString macToString(const std::array<uint8_t, 6>& mac);
    static QString ipToString(uint32_t ip);
    static QString ipv6ToString(const std::array<uint8_t, 16>& ip);
    static QString getEtherTypeName(uint16_t type);
    static QString getIpProtoName(uint8_t proto);

    // Thành viên UI (Giữ nguyên)
    QTableWidget *packetList;
    QTreeWidget *packetDetails;
    QTextEdit *packetBytes;

    // Thành viên Buffer (Bạn đã có, giữ nguyên)
    QList<PacketData> m_packetBuffer;
    QTimer* m_updateTimer;

    // (MỚI) Biến helper để tối ưu hóa việc cuộn
    bool m_isUserAtBottom = true;
};

#endif // PACKETTABLE_HPP
