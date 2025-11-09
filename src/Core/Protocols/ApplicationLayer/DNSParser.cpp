#include "DNSParser.hpp"
#include <arpa/inet.h>      // Cần cho ntohs()
#include <cstring>          // Cần cho memcpy
#include <sstream>
#include <iomanip>
#include <string>           // <-- THÊM MỚI: Cần cho std::string() và std::to_string()

// --- Các hàm trợ giúp nội bộ ---

static void appendTree(std::string& tree, int depth, const std::string& line) {
    tree += std::string(depth * 2, ' ') + line + "\n";
}

// Hàm trợ giúp chuyển số sang hex
template <typename T>
static std::string to_hex(T val) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << val;
    return ss.str();
}

/**
 * @brief Hàm đệ quy quan trọng nhất: đọc tên miền DNS (xử lý cả con trỏ nén).
 */
static std::string parseDNSName(const uint8_t*& reader, const uint8_t* start_of_dns_payload) {
    std::string name;

    while (*reader != 0) {
        // Kiểm tra bit nén (compression pointer)
        if ((*reader & 0xC0) == 0xC0) {
            uint16_t offset = ((*reader & 0x3F) << 8) | *(reader + 1);
            reader += 2;
            const uint8_t* new_reader = start_of_dns_payload + offset;
            name += parseDNSName(new_reader, start_of_dns_payload);
            return name;
        }
        else {
            // Đây là một nhãn (label) bình thường
            uint8_t len = *reader;
            reader++;

            if (!name.empty()) {
                name += ".";
            }

            name.append(reinterpret_cast<const char*>(reader), len);
            reader += len;
        }
    }

    // Đọc byte 0x00 kết thúc
    reader++;
    return name;
}


// Cấu trúc header DNS 12-byte
struct DnsHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t num_questions;
    uint16_t num_answers;
    uint16_t num_authorities;
    uint16_t num_additionals;
};


// --- Triển khai (Implementation) ---

bool DNSParser::parse(ApplicationLayer& app, const uint8_t* data, size_t len) {
    // Header DNS cố định là 12 bytes
    if (len < 12) {
        return false;
    }

    app.protocol = "DNS";

    const DnsHeader* dnsh = (const DnsHeader*)data;

    // Trích xuất header (chuyển sang Host Byte Order)
    app.dns_id = ntohs(dnsh->id);
    uint16_t flags = ntohs(dnsh->flags);
    uint16_t num_questions = ntohs(dnsh->num_questions);

    // Cờ QR (Query/Response): 0 là Query, 1 là Response
    app.is_dns_query = ((flags >> 15) & 0x01) == 0;

    // Bắt đầu đọc con trỏ ngay sau header (12 bytes)
    const uint8_t* reader = data + sizeof(DnsHeader);

    // Phân tích câu hỏi (Question)
    if (num_questions > 0) {
        // Lấy tên
        app.dns_name = parseDNSName(reader, data); // 'data' là đầu payload

        // Đọc QTYPE (2 byte) và QCLASS (2 byte)
        uint16_t qtype, qclass;
        memcpy(&qtype, reader, 2);
        memcpy(&qclass, reader + 2, 2);

        app.dns_type = ntohs(qtype);
        app.dns_class = ntohs(qclass);

        // --- ĐÃ SỬA LỖI (Dòng 116) ---
        // Ép kiểu (cast) vế đầu tiên sang std::string
        app.info = std::string(app.is_dns_query ? "Standard query" : "Standard response") +
                   " " + to_hex(app.dns_id) + " " + app.dns_name;
    } else {
        // --- ĐÃ SỬA LỖI (Dòng 119) ---
        // Ép kiểu (cast) vế đầu tiên sang std::string
        app.info = std::string(app.is_dns_query ? "Standard query" : "Standard response") +
                   " " + to_hex(app.dns_id);
    }

    return true;
}

void DNSParser::appendTreeView(std::string& tree, int depth, const ApplicationLayer& app) {
    appendTree(tree, depth, "Domain Name System (" +
                                std::string(app.is_dns_query ? "Query" : "Response") + ")");
    depth++;

    appendTree(tree, depth, "Transaction ID: " + to_hex(app.dns_id));
    // (Bạn có thể thêm code phân tích 'flags' chi tiết ở đây)

    if (!app.dns_name.empty()) {
        appendTree(tree, depth, "Queries");
        depth++;
        std::string query_line = app.dns_name +
                                 ": type " + std::to_string(app.dns_type) +
                                 ", class " + std::to_string(app.dns_class);
        appendTree(tree, depth, query_line);
    }
    // (Bạn có thể thêm code để phân tích Answers, Authorities... ở đây)
}
