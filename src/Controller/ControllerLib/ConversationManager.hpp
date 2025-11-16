#ifndef CONVERSATIONMANAGER_HPP
#define CONVERSATIONMANAGER_HPP

#include <QObject>
#include <QMap>
#include "../../Common/PacketData.hpp"

/**
 * @brief (MỚI) Dùng để xác định một "cuộc hội thoại" (Stream)
 * Chúng ta sắp xếp (IP thấp, Port thấp) đứng trước để
 * gói tin A->B và B->A đều có cùng một ID.
 */
struct StreamID {
    uint32_t ip1;
    uint32_t ip2;
    uint16_t port1;
    uint16_t port2;
    uint8_t protocol; // 6 (TCP) hoặc 17 (UDP)

    // Hàm so sánh (để QMap có thể dùng)
    bool operator<(const StreamID& other) const {
        if (protocol != other.protocol) return protocol < other.protocol;
        if (ip1 != other.ip1) return ip1 < other.ip1;
        if (ip2 != other.ip2) return ip2 < other.ip2;
        if (port1 != other.port1) return port1 < other.port1;
        return port2 < other.port2;
    }
};

/**
 * @brief (MỚI) Lưu trữ trạng thái của mỗi cuộc hội thoại
 */
struct StreamState {
    bool saw_quic_initial = false;
    // (Trong tương lai, bạn có thể thêm:
    //  uint32_t tcp_initial_seq_a = 0;
    //  uint32_t tcp_initial_seq_b = 0;
    //  bool saw_tcp_syn = false;
    // )
};


class ConversationManager : public QObject
{
    Q_OBJECT
public:
    explicit ConversationManager(QObject *parent = nullptr);

    // (MỚI) Hàm này sẽ được AppController gọi
    void processPackets(const QList<PacketData>& packetBatch);

    // (MỚI) Hàm này xử lý 1 gói VÀ SỬA ĐỔI NÓ
    void processPacket(PacketData& packet); // <-- Tham chiếu (reference)

public slots:
    void clear();

private:
    StreamID getStreamID(const PacketData& packet);

    // "Bộ não" lưu trữ trạng thái
    QMap<StreamID, StreamState> m_streams;
};

#endif // CONVERSATIONMANAGER_HPP
