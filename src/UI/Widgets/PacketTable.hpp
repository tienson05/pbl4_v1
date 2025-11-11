#ifndef PACKETTABLE_HPP
#define PACKETTABLE_HPP

#include <QWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QRegularExpression> // <-- THÊM MỚI
#include "../../Common/PacketData.hpp"

class PacketTable : public QWidget
{
    Q_OBJECT
public:
    explicit PacketTable(QWidget *parent = nullptr);

public slots:
    void addPacket(const PacketData &packet);

    /**
     * @brief (THÊM MỚI) Xóa sạch dữ liệu khỏi bảng và cây.
     */
    void clearData();

private slots:
    void onPacketRowSelected(QTableWidgetItem *item);

private:
    void setupUI();
    void showPacketDetails(const PacketData &packet);
    void showHexDump(const PacketData &packet);
    void addField(QTreeWidgetItem *parent, const QString &name, const QString &value);

    // --- ĐÃ CHUYỂN SANG PUBLIC/STATIC ĐỂ BỘ LỌC CÓ THỂ DÙNG ---
public:
    static QString getSourceAddress(const PacketData &p);
    static QString getDestinationAddress(const PacketData &p);
    static QString getProtocolName(const PacketData &p);
    static QString getInfo(const PacketData &p);

private:
    static QString macToString(const std::array<uint8_t, 6>& mac);
    static QString ipToString(uint32_t ip);
    static QString ipv6ToString(const std::array<uint8_t, 16>& ip);
    static QString getEtherTypeName(uint16_t type);
    static QString getIpProtoName(uint8_t proto);
    // --- KẾT THÚC THAY ĐỔI ---

    QTableWidget *packetList;
    QTreeWidget *packetDetails;
    QTextEdit *packetBytes;
};

#endif // PACKETTABLE_HPP
