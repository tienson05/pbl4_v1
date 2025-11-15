#include "ApplicationParser.hpp"
#include "HTTPParser.hpp"
#include "DNSParser.hpp"
#include <string>

/**
 * @brief (ĐÃ SỬA) Phân tích Tầng 7, phân biệt TCP/UDP
 */
bool ApplicationParser::parse(ApplicationLayer& app, const uint8_t* data, size_t len,
                              uint16_t src_port, uint16_t dest_port, bool is_tcp)
{
    // --- Quyết định dựa trên cổng (Port) ---

    // 1. Kiểm tra HTTP (cổng 80)
    if (src_port == 80 || dest_port == 80) {
        if (is_tcp && len > 0) { // HTTP phải là TCP và có dữ liệu
            return HTTPParser::parse(app, data, len);
        }
    }

    // 2. Kiểm tra DNS (cổng 53)
    if (src_port == 53 || dest_port == 53) {
        if (!is_tcp && len > 0) { // DNS chủ yếu là UDP và có dữ liệu
            return DNSParser::parse(app, data, len);
        }
    }

    // 3. (SỬA LỖI QUAN TRỌNG) Kiểm tra TLS/SSL (HTTPS - Cổng 443)
    if (src_port == 443 || dest_port == 443) {

        // --- LOGIC MỚI ---
        // Chỉ gán nhãn "TLS" nếu:
        // 1. Nó là TCP
        // 2. Nó có mang dữ liệu (len > 0).
        //    (Các gói [SYN], [ACK] trống sẽ có len == 0)

        if (is_tcp && len > 0) {
            app.protocol = "TLS";
            app.info = "Transport Layer Security";
            return true;
        }

        // Nếu là UDP/443 (QUIC) -> không làm gì
        // Nếu là TCP/443 nhưng len == 0 (gói [ACK]) -> không làm gì
        //
        // Khi hàm này 'return false', StatisticsManager và PacketTable
        // sẽ tự động gán nhãn cho nó là "TCP" (đúng như Wireshark)
    }

    // Không tìm thấy parser nào
    return false;
}

void ApplicationParser::appendTreeView(std::string& tree, int depth, const ApplicationLayer& app) {
    // Gọi hàm appendTreeView của parser con tương ứng

    if (app.protocol == "HTTP") {
        HTTPParser::appendTreeView(tree, depth, app);
    }
    else if (app.protocol == "DNS") {
        DNSParser::appendTreeView(tree, depth, app);
    }
    // else if (app.protocol == "TLS") {
    //     // (Tạm thời chưa có)
    // }
}
