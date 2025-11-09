#include "ICMPParser.hpp"
#include <netinet/ip_icmp.h> // Cần cho struct icmphdr
#include <arpa/inet.h>       // Cần cho ntohs
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

// Hàm trợ giúp lấy mô tả Type
static std::string getTypeDescription(uint8_t type) {
    switch (type) {
    case 0:  return "Echo Reply";
    case 3:  return "Destination Unreachable";
    case 5:  return "Redirect";
    case 8:  return "Echo (ping) Request";
    case 11: return "Time Exceeded";
    default: return "Unknown";
    }
}

// --- Triển khai (Implementation) ---

bool ICMPParser::parse(ICMPHeader& icmp, const uint8_t* data, size_t len) {
    // Header ICMP cơ bản là 8 bytes (cho các loại phổ biến)
    if (len < 8) {
        // Một số loại ICMP (như Destination Unreachable) có thể chỉ có 4 byte
        // nhưng chúng ta sẽ giả định 8 byte để lấy ID/Sequence
        if (len < 4) return false;
    }

    // Ép kiểu dữ liệu thô sang cấu trúc ICMP chuẩn của hệ thống
    const struct icmphdr* icmp_hdr = (const struct icmphdr*)data;

    // Trích xuất dữ liệu (chuyển từ Network-Byte-Order sang Host-Byte-Order)
    icmp.type     = icmp_hdr->type;
    icmp.code     = icmp_hdr->code;
    icmp.checksum = ntohs(icmp_hdr->checksum);

    // Chỉ các loại Echo/Reply mới có ID và Sequence
    if (icmp.type == ICMP_ECHO || icmp.type == ICMP_ECHOREPLY) {
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

    appendTree(tree, depth, "Type: " + std::to_string(icmp.type) + " (" + getTypeDescription(icmp.type) + ")");
    appendTree(tree, depth, "Code: " + std::to_string(icmp.code));
    appendTree(tree, depth, "Checksum: " + to_hex(icmp.checksum));

    if (icmp.type == ICMP_ECHO || icmp.type == ICMP_ECHOREPLY) {
        appendTree(tree, depth, "Identifier (ID): " + std::to_string(icmp.id));
        appendTree(tree, depth, "Sequence Number: " + std::to_string(icmp.sequence));
    }
}
