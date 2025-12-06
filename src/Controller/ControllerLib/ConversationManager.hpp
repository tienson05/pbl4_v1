#ifndef CONVERSATIONMANAGER_HPP
#define CONVERSATIONMANAGER_HPP

#include <QObject>
#include <QHash>
#include <QDateTime>
#include <array>
#include "../../Common/PacketData.hpp"

/**
 * @brief Định danh luồng hỗ trợ cả IPv4 và IPv6
 */
struct StreamID {
    // Dùng mảng 16 byte để chứa IP (IPv4 sẽ dùng 4 byte đầu hoặc map sang v6)
    std::array<uint8_t, 16> ip1{};
    std::array<uint8_t, 16> ip2{};
    quint16 port1 = 0;
    quint16 port2 = 0;
    quint8 protocol = 0;
    bool is_ipv6 = false; // Cờ đánh dấu

    // Toán tử so sánh bằng (Bắt buộc cho QHash)
    bool operator==(const StreamID& other) const {
        return ip1 == other.ip1 && ip2 == other.ip2 &&
               port1 == other.port1 && port2 == other.port2 &&
               protocol == other.protocol && is_ipv6 == other.is_ipv6;
    }
};

// Hàm băm (Hash function) cho mảng 16 byte
inline size_t qHash(const std::array<uint8_t, 16>& key, size_t seed = 0) {
    // Hash đơn giản bằng cách XOR các khối 4 byte
    const uint32_t* p = reinterpret_cast<const uint32_t*>(key.data());
    return seed ^ p[0] ^ p[1] ^ p[2] ^ p[3];
}

// Hàm băm chính cho StreamID
inline size_t qHash(const StreamID& key, size_t seed = 0) {
    return qHash(key.ip1, seed) ^ qHash(key.ip2, seed) ^
           qHash(key.port1, seed) ^ qHash(key.port2, seed) ^
           qHash(key.protocol, seed);
}

/**
 * @brief Trạng thái luồng (Thêm TCP)
 */
struct StreamState {
    quint64 stream_index = 0;
    quint64 packet_count = 0;
    QDateTime start_time;
    QDateTime last_activity;

    // --- QUIC State ---
    bool is_quic_confirmed = false;

    // --- TCP State ---
    enum TcpState {
        NONE,
        SYN_SENT,
        SYN_RCVD,
        ESTABLISHED,
        FIN_WAIT,
        CLOSED
    };
    TcpState tcp_state = NONE;
    bool saw_syn = false;      // Đã thấy gói SYN?
    bool saw_syn_ack = false;  // Đã thấy gói SYN-ACK?
};

class ConversationManager : public QObject
{
    Q_OBJECT
public:
    explicit ConversationManager(QObject *parent = nullptr);

    void processPackets(QList<PacketData>& packetBatch);
    void processPacket(PacketData& packet);
    void clear();

private:
    StreamID getStreamID(const PacketData& packet);

    // Hàm phụ trợ để copy IPv4 vào mảng 16 byte
    std::array<uint8_t, 16> ipToBytes(uint32_t ipv4);

    QHash<StreamID, StreamState> m_streams;
    quint64 m_global_stream_counter = 0;
};

#endif // CONVERSATIONMANAGER_HPP
