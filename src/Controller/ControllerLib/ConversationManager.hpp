#ifndef CONVERSATIONMANAGER_HPP
#define CONVERSATIONMANAGER_HPP

#include <QObject>
#include <QHash>      // Thay QMap bằng QHash cho tốc độ O(1)
#include <QDateTime>  // Để theo dõi thời gian luồng
#include "../../Common/PacketData.hpp"

/**
 * @brief Định danh một luồng (5-tuple)
 * Dùng làm Key trong bảng băm (Hash Table)
 */
struct StreamID {
    quint32 ip1 = 0;
    quint32 ip2 = 0;
    quint16 port1 = 0;
    quint16 port2 = 0;
    quint8 protocol = 0; // 6 (TCP) hoặc 17 (UDP)

    // 1. Toán tử so sánh bằng (Bắt buộc cho QHash)
    bool operator==(const StreamID& other) const {
        return ip1 == other.ip1 && ip2 == other.ip2 &&
               port1 == other.port1 && port2 == other.port2 &&
               protocol == other.protocol;
    }
};

// 2. Hàm băm toàn cục (Bắt buộc cho QHash)
inline size_t qHash(const StreamID& key, size_t seed = 0) {
    return qHash(key.ip1, seed) ^ qHash(key.ip2, seed) ^
           qHash(key.port1, seed) ^ qHash(key.port2, seed) ^
           qHash(key.protocol, seed);
}

/**
 * @brief Lưu trữ trạng thái và thống kê của một luồng
 */
struct StreamState {
    quint64 stream_index = 0;      // ID hiển thị (VD: Stream #1, Stream #2...)
    quint64 packet_count = 0;      // Số lượng gói tin trong luồng
    QDateTime start_time;          // Thời gian bắt đầu
    QDateTime last_activity;       // Thời gian gói tin cuối cùng

    // --- Trạng thái giao thức ---
    bool is_quic_confirmed = false; // Đã xác nhận chắc chắn là QUIC (thấy Long Header)
    // bool is_tcp_established = false; // (Dành cho mở rộng TCP sau này)
};

class ConversationManager : public QObject
{
    Q_OBJECT
public:
    explicit ConversationManager(QObject *parent = nullptr);

    // Xử lý một danh sách gói tin (Bỏ const để có thể sửa packet bên trong)
    void processPackets(QList<PacketData>& packetBatch);

    // Xử lý và cập nhật thông tin cho 1 gói tin cụ thể
    void processPacket(PacketData& packet);

    // Xóa dữ liệu (khi Clear hoặc Stop)
    void clear();

private:
    // Tạo ID chuẩn hóa (IP nhỏ đứng trước)
    StreamID getStreamID(const PacketData& packet);

    // Bảng băm lưu trạng thái (Dùng QHash thay vì QMap)
    QHash<StreamID, StreamState> m_streams;

    // Bộ đếm để cấp phát ID luồng (0, 1, 2...)
    quint64 m_global_stream_counter = 0;
};

#endif // CONVERSATIONMANAGER_HPP
