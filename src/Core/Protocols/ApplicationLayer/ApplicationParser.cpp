#include "ApplicationParser.hpp"
#include "HTTPParser.hpp"
#include "DNSParser.hpp" // <-- KÍCH HOẠT

bool ApplicationParser::parse(ApplicationLayer& app, const uint8_t* data, size_t len,
                              uint16_t src_port, uint16_t dest_port)
{
    // --- Quyết định dựa trên cổng (Port) ---

    // 1. Kiểm tra HTTP (cổng 80)
    if (src_port == 80 || dest_port == 80) {
        return HTTPParser::parse(app, data, len);
    }

    // 2. Kiểm tra DNS (cổng 53) (KÍCH HOẠT)
    if (src_port == 53 || dest_port == 53) {
        return DNSParser::parse(app, data, len);
    }

    // 3. Kiểm tra TLS/SSL (HTTPS - Cổng 443)
    if (src_port == 443 || dest_port == 443) {
        app.protocol = "TLS";
        app.info = "Transport Layer Security";
        // (Phân tích TLS rất phức tạp, tạm thời chỉ dán nhãn)
        return true; // Đánh dấu là đã xử lý
    }

    // Không tìm thấy parser nào
    return false;
}

void ApplicationParser::appendTreeView(std::string& tree, int depth, const ApplicationLayer& app) {
    // Gọi hàm appendTreeView của parser con tương ứng

    if (app.protocol == "HTTP") {
        HTTPParser::appendTreeView(tree, depth, app);
    }
    // KÍCH HOẠT
    else if (app.protocol == "DNS") {
        DNSParser::appendTreeView(tree, depth, app);
    }
    // else if (app.protocol == "TLS") {
    //     // (Tạm thời chưa có)
    // }
}
