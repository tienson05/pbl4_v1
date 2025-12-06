#include "ConversationManager.hpp"
#include <algorithm> // std::swap

ConversationManager::ConversationManager(QObject *parent) : QObject(parent)
{
}

void ConversationManager::clear()
{
    m_streams.clear();
    m_global_stream_counter = 0;
}

StreamID ConversationManager::getStreamID(const PacketData& packet)
{
    StreamID id{};

    // Chỉ xử lý IPv4 (bạn có thể mở rộng IPv6 tương tự)
    if (packet.is_ipv4 && (packet.is_tcp || packet.is_udp))
    {
        id.protocol = packet.ipv4.protocol;
        id.ip1 = packet.ipv4.src_ip;
        id.ip2 = packet.ipv4.dest_ip;

        if (packet.is_tcp) {
            id.port1 = packet.tcp.src_port;
            id.port2 = packet.tcp.dest_port;
        } else { // UDP
            id.port1 = packet.udp.src_port;
            id.port2 = packet.udp.dest_port;
        }

        // === CANONICALIZATION (Chuẩn hóa) ===
        // Quy tắc: IP nhỏ đứng trước. Nếu IP bằng nhau, Port nhỏ đứng trước.
        // Điều này đảm bảo A->B và B->A có cùng một ID.
        if (id.ip1 > id.ip2 || (id.ip1 == id.ip2 && id.port1 > id.port2)) {
            std::swap(id.ip1, id.ip2);
            std::swap(id.port1, id.port2);
        }
    }
    return id;
}

void ConversationManager::processPackets(QList<PacketData>& packetBatch)
{
    // Duyệt qua danh sách bằng tham chiếu để có thể sửa đổi packet trực tiếp
    for (PacketData& packet : packetBatch) {
        processPacket(packet);
    }
}

void ConversationManager::processPacket(PacketData& packet)
{
    // 1. Tạo StreamID từ gói tin
    StreamID id = getStreamID(packet);

    // Nếu không phải TCP/UDP (VD: ICMP, ARP), ta bỏ qua việc theo dõi luồng
    if (id.protocol == 0) return;

    // 2. Tìm hoặc Tạo trạng thái (State) cho luồng này
    if (!m_streams.contains(id)) {
        StreamState newState;
        newState.stream_index = m_global_stream_counter++; // Cấp ID mới (0, 1, 2...)
        newState.start_time = QDateTime::currentDateTime(); // Hoặc lấy timestamp gói tin
        newState.is_quic_confirmed = false;
        m_streams.insert(id, newState);
    }

    // Lấy tham chiếu đến trạng thái để cập nhật
    StreamState& state = m_streams[id];
    packet.stream_index = state.stream_index;

    // Cập nhật thống kê cơ bản
    state.packet_count++;
    state.last_activity = QDateTime::currentDateTime(); // Hoặc timestamp gói tin

    // Gán ID luồng vào gói tin (để UI sử dụng: "Follow Stream #12")
    // Giả sử PacketData có trường này (nếu chưa có, bạn nên thêm vào struct PacketData)
    // packet.meta.conversation_id = state.stream_index;

    // ==========================================
    // 3. LOGIC NHẬN DIỆN GIAO THỨC (PROTOCOL HEURISTICS)
    // ==========================================

    // --- QUIC LOGIC ---
    if (packet.is_udp && packet.app.quic_type != ApplicationLayer::NOT_QUIC)
    {
        // Trường hợp A: Gói tin Long Header (Initial/Handshake)
        // Đây là dấu hiệu chắc chắn của QUIC.
        if (packet.app.quic_type == ApplicationLayer::QUIC_LONG_HEADER)
        {
            state.is_quic_confirmed = true; // Đánh dấu luồng này CHÍNH LÀ QUIC
            packet.app.protocol = "QUIC";
            // app.info giữ nguyên từ Parser (VD: "Initial...")
        }
        // Trường hợp B: Gói tin Short Header (Dữ liệu đã mã hóa)
        else if (packet.app.quic_type == ApplicationLayer::QUIC_SHORT_HEADER)
        {
            if (state.is_quic_confirmed) {
                // Đã thấy bắt tay trước đó -> Đây là QUIC
                packet.app.protocol = "QUIC";
                packet.app.info = "Protected Payload";
            } else {
                // Chưa thấy bắt tay -> Có thể là rác UDP hoặc bắt giữa chừng
                // Wireshark sẽ coi là UDP thường nếu không có context.
                packet.app.protocol = "UDP";
                // Xóa info QUIC nếu Parser lỡ gán
                // packet.app.info = "Data";
            }
        }
    }

    // --- TCP LOGIC (Ví dụ mở rộng) ---
    /*
    if (packet.is_tcp) {
        if (packet.tcp.flags & TCP_SYN) {
            state.is_tcp_established = true;
        }
        // ... xử lý FIN/RST
    }
    */
}
