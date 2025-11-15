#include "StatisticsManager.hpp"
#include <sstream>

StatisticsManager::StatisticsManager(QObject *parent)
    : QObject(parent),
    m_totalPackets(0) // <-- KHỞI TẠO
{
}

void StatisticsManager::clear()
{
    m_totalPackets = 0; // <-- RESET
    m_protocolCounts.clear();
    m_sourceIpCounts.clear();
    m_destIpCounts.clear();
}

void StatisticsManager::processPacket(const PacketData &packet)
{
    // --- 1. ĐẾM TỔNG SỐ GÓI TIN ---
    m_totalPackets++; // <-- ĐẾM TỔNG SỐ

    // --- 2. ĐẾM GIAO THỨC ---
    QString finalProto;
    if (!packet.app.protocol.empty()) {
        finalProto = QString::fromStdString(packet.app.protocol);
    }
    else if (packet.is_tcp) { finalProto = "TCP"; }
    else if (packet.is_udp) { finalProto = "UDP"; }
    else if (packet.is_icmp) { finalProto = "ICMP"; }
    else if (packet.is_arp) { finalProto = "ARP"; }
    else { finalProto = "Unknown"; }

    m_protocolCounts[finalProto]++;

    // --- 3. ĐẾM IP ---
    if (packet.is_ipv4) {
        m_sourceIpCounts[ipToString(packet.ipv4.src_ip)]++;
        m_destIpCounts[ipToString(packet.ipv4.dest_ip)]++;
    } else if (packet.is_ipv6) {
        m_sourceIpCounts[ipv6ToString(packet.ipv6.src_ip)]++;
        m_destIpCounts[ipv6ToString(packet.ipv6.dest_ip)]++;
    } else if (packet.is_arp) {
        m_sourceIpCounts[ipToString(packet.arp.sender_ip)]++;
        m_destIpCounts[ipToString(packet.arp.target_ip)]++;
    }
}

// --- CÁC HÀM GETTER ---
QMap<QString, qint64> StatisticsManager::getProtocolCounts() const
{
    return m_protocolCounts;
}

QMap<QString, qint64> StatisticsManager::getSourceIpCounts() const
{
    return m_sourceIpCounts;
}

QMap<QString, qint64> StatisticsManager::getDestIpCounts() const
{
    return m_destIpCounts;
}

qint64 StatisticsManager::getTotalPackets() const
{
    return m_totalPackets; // <-- TRẢ VỀ TỔNG SỐ
}


// --- CÁC HÀM HỖ TRỢ ---
QString StatisticsManager::ipToString(uint32_t ip_host_order)
{
    return QString("%1.%2.%3.%4")
    .arg((ip_host_order) & 0xFF)
        .arg((ip_host_order >> 8) & 0xFF)
        .arg((ip_host_order >> 16) & 0xFF)
        .arg((ip_host_order >> 24) & 0xFF);
}

QString StatisticsManager::ipv6ToString(const std::array<uint8_t, 16>& ip)
{
    QStringList parts;
    for (int i = 0; i < 16; i += 2) {
        uint16_t part = (ip[i] << 8) | ip[i + 1];
        parts << QString("%1").arg(part, 0, 16);
    }
    return parts.join(":");
}
