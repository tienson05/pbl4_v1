#include "StatisticsManager.hpp"

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
QVector<QPointF> StatisticsManager::calculateIOGraphData(const QList<PacketData>& packets, int intervalMs, bool modeBytes)
{
    QVector<QPointF> points;
    if (packets.isEmpty()) return points;

    // 1. Lấy thời gian bắt đầu (gói tin đầu tiên làm mốc 0)
    double startTime = packets.first().timestamp.tv_sec +
                       packets.first().timestamp.tv_nsec / 1.0e9;

    // Map: Key = chỉ số khoảng thời gian (0, 1, 2...), Value = Tổng lưu lượng
    QMap<int, double> timeBuckets;
    int maxIndex = 0;

    // 2. Gom nhóm dữ liệu
    for (const auto& pkt : packets) {
        double pktTime = pkt.timestamp.tv_sec + pkt.timestamp.tv_nsec / 1.0e9;
        double diff = pktTime - startTime;

        if (diff < 0) diff = 0; // An toàn

        int index = static_cast<int>(diff * 1000.0 / intervalMs);
        if (index > maxIndex) maxIndex = index;

        double valueToAdd = modeBytes ? pkt.wire_length : 1.0;
        timeBuckets[index] += valueToAdd;
    }

    // 3. Tạo điểm dữ liệu (điền cả những khoảng trống bằng 0)
    for (int i = 0; i <= maxIndex; ++i) {
        double timeX = i * (intervalMs / 1000.0); // Quy đổi ra giây cho trục X
        double valY = timeBuckets.value(i, 0.0);
        points.append(QPointF(timeX, valY));
    }

    return points;
}

void StatisticsManager::processPackets(const QList<PacketData> &packetBatch)
{
    for (const PacketData &packet : packetBatch) {
        // Gọi hàm xử lý 1 gói
        this->processPacket(packet);
    }
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
