#include "ConversationManager.hpp"
#include <algorithm> // std::swap
#include <cstring>   // memcpy

ConversationManager::ConversationManager(QObject *parent) : QObject(parent)
{
}

void ConversationManager::clear()
{
    m_streams.clear();
    m_global_stream_counter = 0;
}

// Hàm phụ trợ: Chuyển IPv4 uint32 thành std::array
std::array<uint8_t, 16> ConversationManager::ipToBytes(uint32_t ipv4) {
    std::array<uint8_t, 16> arr{}; // Init 0
    // Copy 4 byte IPv4 vào đầu mảng (hoặc có thể map sang IPv6 ::ffff:...)
    // Ở đây ta copy vào 4 byte đầu cho đơn giản
    arr[0] = ipv4 & 0xFF;
    arr[1] = (ipv4 >> 8) & 0xFF;
    arr[2] = (ipv4 >> 16) & 0xFF;
    arr[3] = (ipv4 >> 24) & 0xFF;
    return arr;
}

StreamID ConversationManager::getStreamID(const PacketData& packet)
{
    StreamID id{};

    // 1. Lấy thông tin Protocol và Port
    if (packet.is_tcp) {
        id.protocol = 6; // TCP
        id.port1 = packet.tcp.src_port;
        id.port2 = packet.tcp.dest_port;
    } else if (packet.is_udp) {
        id.protocol = 17; // UDP
        id.port1 = packet.udp.src_port;
        id.port2 = packet.udp.dest_port;
    } else {
        return id; // Không hỗ trợ protocol khác lúc này
    }

    // 2. Lấy thông tin IP (Hỗ trợ v4 và v6)
    if (packet.is_ipv4) {
        id.is_ipv6 = false;
        id.ip1 = ipToBytes(packet.ipv4.src_ip);
        id.ip2 = ipToBytes(packet.ipv4.dest_ip);
    }
    else if (packet.is_ipv6) {
        id.is_ipv6 = true;
        id.ip1 = packet.ipv6.src_ip; // PacketData đã dùng std::array<uint8_t, 16>
        id.ip2 = packet.ipv6.dest_ip;
    } else {
        return id;
    }

    // 3. CANONICALIZATION (Chuẩn hóa: IP nhỏ đứng trước)
    // std::array hỗ trợ so sánh từ điển (lexicographical compare)
    if (id.ip1 > id.ip2 || (id.ip1 == id.ip2 && id.port1 > id.port2)) {
        std::swap(id.ip1, id.ip2);
        std::swap(id.port1, id.port2);
    }

    return id;
}

void ConversationManager::processPackets(QList<PacketData>& packetBatch)
{
    for (PacketData& packet : packetBatch) {
        processPacket(packet);
    }
}

void ConversationManager::processPacket(PacketData& packet)
{
    StreamID id = getStreamID(packet);
    if (id.protocol == 0) return;

    // Tạo trạng thái mới nếu chưa có
    if (!m_streams.contains(id)) {
        StreamState newState;
        newState.stream_index = m_global_stream_counter++;
        newState.start_time = QDateTime::currentDateTime();
        newState.tcp_state = StreamState::NONE; // Init TCP state
        m_streams.insert(id, newState);
    }

    StreamState& state = m_streams[id];

    // Cập nhật thống kê
    state.packet_count++;
    state.last_activity = QDateTime::currentDateTime();

    // Gán Stream Index vào gói tin
    packet.stream_index = state.stream_index;

    // ============================
    // LOGIC TCP (Handshake Tracking)
    // ============================
    if (packet.is_tcp) {
        // Cập nhật thông tin hiển thị (Info Column) dựa trên cờ
        // (Phần này giúp UI hiển thị giống Wireshark: [SYN], [ACK]...)
        QString flagsStr = "";
        if (packet.tcp.flags & TCPHeader::SYN) flagsStr += "SYN, ";
        if (packet.tcp.flags & TCPHeader::FIN) flagsStr += "FIN, ";
        if (packet.tcp.flags & TCPHeader::RST) flagsStr += "RST, ";
        if (packet.tcp.flags & TCPHeader::PSH) flagsStr += "PSH, ";
        if (packet.tcp.flags & TCPHeader::ACK) flagsStr += "ACK, ";
        if (!flagsStr.isEmpty()) flagsStr.chop(2); // Xóa dấu phẩy cuối

        // Logic trạng thái kết nối
        if (packet.tcp.flags & TCPHeader::SYN) {
            if (packet.tcp.flags & TCPHeader::ACK) {
                // SYN-ACK: Server phản hồi
                state.saw_syn_ack = true;
                if (state.tcp_state == StreamState::SYN_SENT) {
                    state.tcp_state = StreamState::SYN_RCVD;
                }
            } else {
                // SYN: Client bắt đầu
                state.saw_syn = true;
                state.tcp_state = StreamState::SYN_SENT;
            }
        }
        else if ((packet.tcp.flags & TCPHeader::ACK) && !(packet.tcp.flags & TCPHeader::SYN)) {
            // ACK (Gói thứ 3 của handshake)
            if (state.saw_syn && state.saw_syn_ack && state.tcp_state == StreamState::SYN_RCVD) {
                state.tcp_state = StreamState::ESTABLISHED;
            }
        }
        else if (packet.tcp.flags & TCPHeader::FIN) {
            state.tcp_state = StreamState::FIN_WAIT;
        }
        else if (packet.tcp.flags & TCPHeader::RST) {
            state.tcp_state = StreamState::CLOSED;
        }

        // Cập nhật Info nếu chưa có Application Info
        // (Ví dụ: thay vì hiện mỗi "TCP", hiện "60442 -> 443 [SYN] Seq=0...")
        /* Lưu ý: Logic tạo string info chi tiết đã có trong PacketTable::getInfo,
         nhưng ở đây bạn có thể bổ sung trạng thái luồng nếu muốn.
        */
    }

    // ============================
    // LOGIC QUIC
    // ============================
    else if (packet.is_udp && packet.app.quic_type != ApplicationLayer::NOT_QUIC)
    {
        if (packet.app.quic_type == ApplicationLayer::QUIC_LONG_HEADER) {
            state.is_quic_confirmed = true;
            packet.app.protocol = "QUIC";
        }
        else if (packet.app.quic_type == ApplicationLayer::QUIC_SHORT_HEADER) {
            if (state.is_quic_confirmed) {
                packet.app.protocol = "QUIC";
                packet.app.info = "Protected Payload";
            } else {
                packet.app.protocol = "UDP";
            }
        }
    }
}
