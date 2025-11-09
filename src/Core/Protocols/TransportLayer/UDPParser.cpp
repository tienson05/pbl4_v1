#include "UDPParser.hpp"
#include <netinet/udp.h>    // Cần cho struct udphdr
#include <arpa/inet.h>      // Cần cho ntohs()
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

// Hàm trợ giúp lấy tên cổng (dịch vụ)
static std::string getPortName(uint16_t port) {
    switch (port) {
    case 53:   return "dns";
    case 67:   return "bootps (DHCP Server)";
    case 68:   return "bootpc (DHCP Client)";
    case 123:  return "ntp";
    case 161:  return "snmp";
    case 5353: return "mdns";
    default: return std::to_string(port);
    }
}

// --- Triển khai (Implementation) ---

bool UDPParser::parse(UDPHeader& udp, const uint8_t* data, size_t len) {
    // Header UDP cố định là 8 bytes
    if (len < 8) {
        return false;
    }

    // Ép kiểu dữ liệu thô sang cấu trúc UDP chuẩn của hệ thống
    const struct udphdr* udp_hdr = (const struct udphdr*)data;

    // Trích xuất dữ liệu (chuyển từ Network-Byte-Order sang Host-Byte-Order)
    udp.src_port  = ntohs(udp_hdr->uh_sport);
    udp.dest_port = ntohs(udp_hdr->uh_dport);
    udp.length    = ntohs(udp_hdr->uh_ulen);
    udp.checksum  = ntohs(udp_hdr->uh_sum);

    // Kiểm tra độ dài (ít nhất là 8 bytes)
    if (udp.length < 8) {
        return false;
    }

    return true;
}

void UDPParser::appendTreeView(std::string& tree, int depth, const UDPHeader& udp) {
    appendTree(tree, depth, "User Datagram Protocol");
    depth++;

    appendTree(tree, depth, "Source Port: " + getPortName(udp.src_port));
    appendTree(tree, depth, "Destination Port: " + getPortName(udp.dest_port));
    appendTree(tree, depth, "Length: " + std::to_string(udp.length));
    appendTree(tree, depth, "Checksum: " + to_hex(udp.checksum));
}
