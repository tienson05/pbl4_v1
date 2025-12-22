#include "ICMPParser.hpp"
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sstream>
#include <iomanip>

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
std::string ICMPParser::getTypeString(uint8_t type) {
    switch (type) {
    case 0:  return "Echo Reply";
    case 3:  return "Destination Unreachable";
    case 5:  return "Redirect";
    case 8:  return "Echo (ping) Request";
    case 11: return "Time Exceeded";
        // --- ICMPv6 (Neighbor Discovery Protocol) ---
    case 128: return "Echo Request (IPv6)";
    case 129: return "Echo Reply (IPv6)";
    case 133: return "Router Solicitation";

    // ĐÂY LÀ CÁI BẠN ĐANG THIẾU (Type 134)
    case 134: return "Router Advertisement";

    case 135: return "Neighbor Solicitation";
    case 136: return "Neighbor Advertisement";

    default: return "Unknown Type";
    }
}

// --- Triển khai (Implementation) ---

bool ICMPParser::parse(ICMPHeader& icmp, const uint8_t* data, size_t len) {

    // Các loại khác (ví dụ: Destination Unreachable) cần ít nhất 4 byte
    if (len < 4) {
        return false;
    }

    // Ép kiểu dữ liệu thô sang cấu trúc ICMP chuẩn của hệ thống
    const struct icmphdr* icmp_hdr = (const struct icmphdr*)data;

    // Trích xuất dữ liệu (chuyển từ Network-Byte-Order sang Host-Byte-Order)
    icmp.type     = icmp_hdr->type;
    icmp.code     = icmp_hdr->code;
    icmp.checksum = ntohs(icmp_hdr->checksum);

    if ((icmp.type == ICMP_ECHO || icmp.type == ICMP_ECHOREPLY) && len >= 8) {
        icmp.id       = ntohs(icmp_hdr->un.echo.id);
        icmp.sequence = ntohs(icmp_hdr->un.echo.sequence);
    } else {
        icmp.id = 0;
        icmp.sequence = 0;
    }

    return true;
}

void ICMPParser::appendTreeView(std::string& tree, int depth, const ICMPHeader& icmp) {
    appendTree(tree, depth, "Internet Control Message Protocol");
    depth++;

    appendTree(tree, depth, "Type: " + std::to_string(icmp.type) + " (" + getTypeString(icmp.type) + ")");
    appendTree(tree, depth, "Code: " + std::to_string(icmp.code));
    appendTree(tree, depth, "Checksum: " + to_hex(icmp.checksum));

    if (icmp.type == ICMP_ECHO || icmp.type == ICMP_ECHOREPLY) {
        appendTree(tree, depth, "Identifier (ID): " + std::to_string(icmp.id) + " (" + to_hex(icmp.id) + ")");
        appendTree(tree, depth, "Sequence Number: " + std::to_string(icmp.sequence) + " (" + to_hex(icmp.sequence) + ")");
    }
}
