#ifndef STATISTICSMANAGER_HPP
#define STATISTICSMANAGER_HPP

#include <QObject>
#include <QMap>
#include <QString>
#include <QList> // <-- THÊM MỚI
#include "../../Common/PacketData.hpp"

class StatisticsManager : public QObject
{
    Q_OBJECT
public:
    explicit StatisticsManager(QObject *parent = nullptr);

    // --- CÁC HÀM GETTER ---
    QMap<QString, qint64> getProtocolCounts() const;
    QMap<QString, qint64> getSourceIpCounts() const;
    QMap<QString, qint64> getDestIpCounts() const;
    qint64 getTotalPackets() const;

public slots:
    // (Hàm cũ xử lý 1 gói)
    void processPacket(const PacketData &packet);

    // --- (THÊM MỚI) Hàm xử lý 1 LÔ (batch) ---
    void processPackets(const QList<PacketData> &packetBatch);

    void clear();

private:
    // --- 4 BỘ ĐẾM ---
    qint64 m_totalPackets;
    QMap<QString, qint64> m_protocolCounts;
    QMap<QString, qint64> m_sourceIpCounts;
    QMap<QString, qint64> m_destIpCounts;

    // --- HÀM HỖ TRỢ ---
    static QString ipToString(uint32_t ip_host_order);
    static QString ipv6ToString(const std::array<uint8_t, 16>& ip);
};

#endif // STATISTICSMANAGER_HPP
