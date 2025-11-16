#include "ConversationManager.hpp"
#include <algorithm> // (Cần cho std::swap)

ConversationManager::ConversationManager(QObject *parent) : QObject(parent)
{
}

void ConversationManager::clear()
{
    m_streams.clear();
}

/**
 * @brief (MỚI) Tạo ID cho một luồng (stream)
 */
StreamID ConversationManager::getStreamID(const PacketData& packet)
{
    StreamID id{};
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

        // Sắp xếp lại (IP thấp/Port thấp đứng trước)
        // để luồng A->B và B->A có cùng ID
        if (id.ip1 > id.ip2 || (id.ip1 == id.ip2 && id.port1 > id.port2)) {
            std::swap(id.ip1, id.ip2);
            std::swap(id.port1, id.port2);
        }
    }
    // (Bạn có thể thêm 'else if (packet.is_ipv6)' ở đây)

    return id;
}


void ConversationManager::processPackets(const QList<PacketData>& packetBatch)
{
    // (Hàm này không được dùng trong kiến trúc mới,
    //  nhưng chúng ta giữ lại để tương thích)
    // (AppController sẽ gọi processPacket() trong vòng lặp)
}

/**
 * @brief (MỚI) Xử lý 1 gói, SỬA ĐỔI nó dựa trên trạng thái
 */
void ConversationManager::processPacket(PacketData& packet)
{
    // === LOGIC QUIC (THEO DÕI TRẠNG THÁI) ===

    // 1. Nếu ApplicationParser đã đánh dấu nó là QUIC...
    if (packet.app.quic_type != ApplicationLayer::NOT_QUIC)
    {
        // 2. Lấy ID luồng và tìm trạng thái
        StreamID id = getStreamID(packet);
        if (id.protocol == 0) return; // Không phải luồng

        // 3. Tìm (hoặc tạo) trạng thái cho luồng này
        StreamState& state = m_streams[id];

        // 4. Nếu đây là gói MỞ ĐẦU (Long Header)
        if (packet.app.quic_type == ApplicationLayer::QUIC_LONG_HEADER)
        {
            // a. Đánh dấu là chúng ta đã thấy
            state.saw_quic_initial = true;
            // b. Ra quyết định cuối cùng: Đây là QUIC
            packet.app.protocol = "QUIC";
            // (app.info đã được ApplicationParser gán)
        }
        // 5. Nếu đây là gói (Short Header)
        else if (packet.app.quic_type == ApplicationLayer::QUIC_SHORT_HEADER)
        {
            // --- (LOGIC CỦA WIRESHARK) ---
            // a. Kiểm tra xem chúng ta đã thấy gói MỞ ĐẦU chưa
            if (state.saw_quic_initial) {
                // b. Đã thấy -> Đây là QUIC
                packet.app.protocol = "QUIC";
                packet.app.info = "Protected Payload (Short)";
                // (app.info đã được ApplicationParser gán)
            }
            // c. CHƯA thấy (Giống Wireshark) -> Bỏ qua
            //    (packet.app.protocol sẽ rỗng, và PacketTable
            //     sẽ gán nhãn là "UDP" - chính xác như bạn muốn!)
        }
    }
}
