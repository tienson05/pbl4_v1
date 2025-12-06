#include "DNSParser.hpp"
#include <arpa/inet.h>    // Cần cho ntohs(), ntohl() và inet_ntoa()
#include <cstring>        // Cần cho memcpy
#include <sstream>
#include <iomanip>
#include <string>         // Cần cho std::string() và std::to_string()

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

// Hàm helper để chuyển Loại DNS (Type) sang chuỗi
static std::string getDnsTypeName(uint16_t type) {
    switch (type) {
    case 1:   return "A";
    case 2:   return "NS";
    case 5:   return "CNAME";
    case 6:   return "SOA";
    case 12:  return "PTR";      // Quan trọng với MDNS
    case 15:  return "MX";
    case 16:  return "TXT";      // Thông tin thiết bị MDNS
    case 28:  return "AAAA";     // IPv6
    case 33:  return "SRV";      // Dịch vụ MDNS
    case 47:  return "NSEC";
    case 255: return "ANY";
    default:  return "Type(" + std::to_string(type) + ")";
    }
}

/**
 * @brief Hàm đệ quy quan trọng nhất: đọc tên miền DNS (xử lý cả con trỏ nén).
 */
static std::string parseDNSName(const uint8_t*& reader, const uint8_t* start_of_dns_payload) {
    std::string name;
    int jumps = 0; // Chống vòng lặp vô tận

    // Tạo một con trỏ tạm để đọc mà không làm thay đổi 'reader' chính khi nhảy
    const uint8_t* current_reader = reader;

    while (*current_reader != 0) {
        // Kiểm tra bit nén (compression pointer)
        if ((*current_reader & 0xC0) == 0xC0) {
            uint16_t offset = ((*current_reader & 0x3F) << 8) | *(current_reader + 1);

            // Quan trọng: Chỉ tăng 'reader' chính 2 byte (để bỏ qua con trỏ)
            // Lần gọi đệ quy tiếp theo sẽ dùng con trỏ mới từ 'start_of_dns_payload'
            reader = current_reader + 2;;

            const uint8_t* new_reader = start_of_dns_payload + offset;
            name += parseDNSName(new_reader, start_of_dns_payload); // Đệ quy
            return name; // Kết thúc ngay sau khi nhảy
        }
        else {
            // Đây là một nhãn (label) bình thường
            uint8_t len = *current_reader;
            current_reader++; // Bỏ qua byte độ dài

            if (!name.empty()) {
                name += ".";
            }

            name.append(reinterpret_cast<const char*>(current_reader), len);
            current_reader += len;
        }
    }

    // Đọc byte 0x00 kết thúc
    current_reader++;
    // Cập nhật 'reader' chính bằng con trỏ tạm
    reader = current_reader;
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
    if (len < 12) return false;

    if (app.protocol.empty()) app.protocol = "DNS";

    const DnsHeader* dnsh = (const DnsHeader*)data;

    app.dns_id = ntohs(dnsh->id);
    uint16_t flags = ntohs(dnsh->flags);
    uint16_t num_questions = ntohs(dnsh->num_questions);
    uint16_t num_answers = ntohs(dnsh->num_answers);
    uint16_t num_authorities = ntohs(dnsh->num_authorities);
    uint16_t num_additionals = ntohs(dnsh->num_additionals);

    app.is_dns_query = ((flags >> 15) & 0x01) == 0;

    // 1. Xây dựng phần mở đầu: "Standard query 0x0000"
    std::stringstream info_ss;
    info_ss << (app.is_dns_query ? "Standard query" : "Standard response")
            << " " << to_hex(app.dns_id);

    const uint8_t* reader = data + sizeof(DnsHeader);

    // 2. Phân tích Questions (Xử lý dấu phẩy và QU/QM)
    for(int i = 0; i < num_questions; ++i) {
        // Nếu đây là câu hỏi thứ 2 trở đi, thêm dấu phẩy ngăn cách
        if (i > 0) {
            info_ss << ", ";
        }


        // Đọc tên miền
        std::string qName = parseDNSName(reader, data);

        // Đọc Type và Class
        if (reader + 4 > data + len) break; // Kiểm tra tràn bộ nhớ

        uint16_t qtype, qclass_raw;
        memcpy(&qtype, reader, 2); qtype = ntohs(qtype);
        memcpy(&qclass_raw, reader + 2, 2); qclass_raw = ntohs(qclass_raw);

        reader += 4; // Tăng con trỏ qua Type(2) và Class(2)

        // --- LOGIC WIRESHARK "QU" QUESTION ---
        // Bit cao nhất của Class quy định Unicast (QU) hay Multicast (QM)
        bool isQU = (qclass_raw & 0x8000);

        // Format: " PTR _companion... "
        info_ss << " " << getDnsTypeName(qtype) << " " << qName;

        // Nếu là MDNS, hiển thị thêm trạng thái QU/QM
        if (app.protocol == "MDNS") {
            if (isQU) {
                info_ss << ", \"QU\" question";
            } else {
                info_ss << ", \"QM\" question";
            }
        }
        // -------------------------------------

        // Lưu thông tin câu hỏi đầu tiên vào struct (để dùng cho packet details tree)
        if (i == 0) {
            app.dns_name = qName;
            app.dns_type = qtype;
            app.dns_class = qclass_raw & 0x7FFF; // Bỏ bit QU để lấy Class thật
        }
    }

    // 3. Phân tích Resource Records (Answers/Authorities/Additionals)
    auto parseRRs = [&](uint16_t count) {
        for (int i = 0; i < count; ++i) {
            if (info_ss.tellp() > 0) info_ss << ", "; // Ngăn cách bằng dấu phẩy

            std::string name = parseDNSName(reader, data);

            if (reader + 10 > data + len) return;

            uint16_t type, rclass_raw, rdlen;
            uint32_t ttl;
            memcpy(&type, reader, 2); type = ntohs(type);
            memcpy(&rclass_raw, reader + 2, 2); rclass_raw = ntohs(rclass_raw);
            memcpy(&ttl, reader + 4, 4); ttl = ntohl(ttl);
            memcpy(&rdlen, reader + 8, 2); rdlen = ntohs(rdlen);

            reader += 10;

            if (reader + rdlen > data + len) return;

            // Bit cache flush (MDNS)
            bool cacheFlush = (rclass_raw & 0x8000) != 0;

            info_ss << " " << getDnsTypeName(type);
            if (cacheFlush) info_ss << " cache flush";
            info_ss << " " << name;

            // Parse chi tiết SRV/A/PTR nếu cần
            if (type == 33 && rdlen >= 6) { // SRV
                uint16_t priority, weight, port;
                memcpy(&priority, reader, 2); priority = ntohs(priority);
                memcpy(&weight, reader + 2, 2); weight = ntohs(weight);
                memcpy(&port, reader + 4, 2); port = ntohs(port);
                info_ss << " SRV 0 " << weight << " " << port;
                // target name parse ở đây sẽ phức tạp hơn chút
            }

            reader += rdlen;
        }
    };

    parseRRs(num_answers);
    parseRRs(num_authorities);
    parseRRs(num_additionals);

    app.info = info_ss.str();
    return true;
}

void DNSParser::appendTreeView(std::string& tree, int depth, const ApplicationLayer& app) {
    appendTree(tree, depth, "Domain Name System (" +
                                std::string(app.is_dns_query ? "Query" : "Response") + ")");
    depth++;

    appendTree(tree, depth, "Transaction ID: " + to_hex(app.dns_id));

    if (!app.dns_name.empty()) {
        appendTree(tree, depth, "Queries");
        // Có thể mở rộng để hiển thị nhiều câu hỏi trong Tree View
        std::string query_line = "  " + app.dns_name +
                                 ": type " + getDnsTypeName(app.dns_type) +
                                 ", class " + ((app.protocol == "MDNS") ? "IN" : "IN");
        appendTree(tree, depth, query_line);
    }
}
