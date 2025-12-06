#include "ARPParser.hpp"
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <cstring>
#include <sstream>
#include <iomanip>

// --- Các hàm trợ giúp nội bộ ---

static void appendTree(std::string& tree, int depth, const std::string& line) {
    tree += std::string(depth * 2, ' ') + line + "\n";
}

// Hàm trợ giúp để chuyển đổi MAC
static std::string macToString(const std::array<uint8_t, 6>& mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

// Hàm trợ giúp để chuyển đổi IP (uint32_t)
// (Lưu ý: Hàm này giả định ip_val là Network Order)
static std::string ipToString(uint32_t ip_val) {
    char buf[INET_ADDRSTRLEN];
    struct in_addr addr;
    addr.s_addr = ip_val; // Dùng trực tiếp (đã là network order)
    inet_ntop(AF_INET, &addr, buf, sizeof(buf));
    return std::string(buf);
}

// Hàm trợ giúp chuyển số sang hex
template <typename T>
static std::string to_hex(T val) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << val;
    return ss.str();
}

// --- Triển khai (Implementation) ---

bool ARPParser::parse(ARPHeader& arp, const uint8_t* data, size_t len) {
    // Gói ARP chuẩn cho (Ethernet/IPv4) là 28 bytes
    if (len < 28) {
        return false;
    }

    // Ép kiểu dữ liệu thô sang cấu trúc ARP chuẩn của hệ thống
    const struct ether_arp* arp_hdr = (const struct ether_arp*)data;

    // Trích xuất dữ liệu (chuyển từ Network-Byte-Order sang Host-Byte-Order)
    arp.hardware_type = ntohs(arp_hdr->ea_hdr.ar_hrd);
    arp.protocol_type = ntohs(arp_hdr->ea_hdr.ar_pro);
    arp.hardware_size = arp_hdr->ea_hdr.ar_hln;
    arp.protocol_size = arp_hdr->ea_hdr.ar_pln;
    arp.opcode = ntohs(arp_hdr->ea_hdr.ar_op);

    // Sao chép (copy) mảng
    memcpy(arp.sender_mac.data(), arp_hdr->arp_sha, 6);
    memcpy(arp.target_mac.data(), arp_hdr->arp_tha, 6);

    // IP trong ether_arp là mảng byte, cần chuyển sang uint32_t
    uint32_t sender_ip_net, target_ip_net;
    memcpy(&sender_ip_net, arp_hdr->arp_spa, 4);
    memcpy(&target_ip_net, arp_hdr->arp_tpa, 4);


    // (Lưu IP ở dạng Network Byte Order để khớp với IPv4Parser)
    arp.sender_ip = sender_ip_net;
    arp.target_ip = target_ip_net;
    // --- KẾT THÚC SỬA LỖI ---

    // Chỉ chấp nhận ARP cho Ethernet và IPv4
    if (arp.hardware_type != ARPHRD_ETHER ||
        arp.protocol_type != 0x0800 || // 0x0800 là IPv4
        arp.hardware_size != 6 ||
        arp.protocol_size != 4) {
        return false; // Đây là một loại ARP khác
    }

    return true;
}

void ARPParser::appendTreeView(std::string& tree, int depth, const ARPHeader& arp) {
    std::string op_str = (arp.opcode == 1) ? "Request" : (arp.opcode == 2 ? "Reply" : "Unknown");
    appendTree(tree, depth, "Address Resolution Protocol (" + op_str + ")");
    depth++;

    appendTree(tree, depth, "Hardware type: " + (arp.hardware_type == 1 ? "Ethernet (1)" : std::to_string(arp.hardware_type)));
    appendTree(tree, depth, "Protocol type: " + to_hex(arp.protocol_type) + " (IPv4)");
    appendTree(tree, depth, "Hardware size: " + std::to_string(arp.hardware_size));
    appendTree(tree, depth, "Protocol size: " + std::to_string(arp.protocol_size));
    appendTree(tree, depth, "Opcode: " + op_str + " (" + std::to_string(arp.opcode) + ")");
    appendTree(tree, depth, "Sender MAC: " + macToString(arp.sender_mac));
    appendTree(tree, depth, "Sender IP: " + ipToString(arp.sender_ip));
    appendTree(tree, depth, "Target MAC: " + macToString(arp.target_mac));
    appendTree(tree, depth, "Target IP: " + ipToString(arp.target_ip));
}
