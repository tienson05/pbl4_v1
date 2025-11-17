#ifndef PACKETTABLE_HPP
#define PACKETTABLE_HPP

#include <QWidget>
#include <QList>
#include <QTimer>
#include <QRegularExpression>
#include <QTreeWidget>
#include "../../Common/PacketData.hpp"

// Khai báo trước (Forward declarations)
class QTableWidget;
class QTextEdit;
class QTableWidgetItem;
class QTreeWidgetItem; // <-- THÊM MỚI

class PacketTable : public QWidget
{
    Q_OBJECT

public:
    explicit PacketTable(QWidget *parent = nullptr);

public slots:
    void onPacketReceived(const PacketData &packet);
    void onPacketsReceived(QList<PacketData>* packets);
    void clearData();

private slots:
    void processPacketChunk();
    void onPacketRowSelected(QTableWidgetItem *item);

    /**
     * @brief (MỚI) Slot này được gọi khi nhấp vào một dòng
     * trong cây chi tiết (Packet Details).
     */
    void onDetailRowSelected(QTreeWidgetItem *item, int column);

private:
    void insertPacketRow(const PacketData& packet);
    void setupUI();

    /**
     * @brief (ĐÃ SỬA) Hàm này giờ cần biết packet
     * để tính toán offset (vị trí byte).
     */
    void showPacketDetails(const PacketData &packet);

    /**
     * @brief (ĐÃ SỬA) Hàm này giờ nhận offset và length
     * để tô đậm (highlight).
     */
    void showHexDump(const PacketData &packet, int highlight_offset = -1, int highlight_len = 0);

    /**
     * @brief (ĐÃ SỬA) Hàm helper này giờ lưu trữ
     * siêu dữ liệu (metadata) về byte.
     */
    void addField(QTreeWidgetItem *parent, const QString &name, const QString &value, int offset = -1, int length = 0);


public: // (Các hàm static)
    static bool packetMatchesFilter(const PacketData &packet, const QString &filterText);
    static QString getSourceAddress(const PacketData &p);
    static QString getDestinationAddress(const PacketData &p);
    static QString getProtocolName(const PacketData &p);
    static QString getInfo(const PacketData &p);

private: // (Các hàm static)
    static QString macToString(const std::array<uint8_t, 6>& mac);
    static QString ipToString(uint32_t ip);
    static QString ipv6ToString(const std::array<uint8_t, 16>& ip);
    static QString getEtherTypeName(uint16_t type);
    static QString getIpProtoName(uint8_t proto);

    // Thành viên UI
    QTableWidget *packetList;
    QTreeWidget *packetDetails;
    QTextEdit *packetBytes;

    // (MỚI) Lưu gói tin đang được chọn
    PacketData m_currentSelectedPacket;

    // (Các thành viên cho QTimer giữ nguyên)
    QList<PacketData> m_packetBuffer;
    QTimer* m_updateTimer;
    bool m_isUserAtBottom = true;
};

#endif // PACKETTABLE_HPP
