#include "TCPParser.hpp"
#include <netinet/tcp.h>    // Cần cho struct tcphdr
#include <arpa/inet.h>      // Cần cho ntohs(), ntohl()
#include <cstring>          // Cần cho memcpy
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
    case 80:   return "http";
    case 443:  return "https (tls)";
    case 20:   return "ftp-data";
    case 21:   return "ftp";
    case 22:   return "ssh";
    case 23:   return "telnet";
    case 25:   return "smtp";
    case 53:   return "dns";
    case 110:  return "pop3";
    default: return std::to_string(port);
    }
}

// --- Triển khai (Implementation) ---

bool TCPParser::parse(TCPHeader& tcp, const uint8_t* data, size_t len) {
    // Header TCP tối thiểu là 20 bytes
    if (len < 20) {
        return false;
    }

    // Ép kiểu dữ liệu thô sang cấu trúc TCP chuẩn của hệ thống
    const struct tcphdr* tcp_hdr = (const struct tcphdr*)data;

    // Trích xuất dữ liệu (chuyển từ Network-Byte-Order sang Host-Byte-Order)
    tcp.src_port      = ntohs(tcp_hdr->th_sport);
    tcp.dest_port     = ntohs(tcp_hdr->th_dport);
    tcp.seq_num       = ntohl(tcp_hdr->th_seq);
    tcp.ack_num       = ntohl(tcp_hdr->th_ack);
    tcp.data_offset   = tcp_hdr->th_off; // Số lượng từ 32-bit (ví dụ: 5 = 20 bytes)
    tcp.flags         = tcp_hdr->th_flags;
    tcp.window        = ntohs(tcp_hdr->th_win);
    tcp.checksum      = ntohs(tcp_hdr->th_sum);
    tcp.urgent_pointer = ntohs(tcp_hdr->th_urp);

    // Kiểm tra độ dài header hợp lệ (tối thiểu 5 * 4 = 20 bytes)
    if (tcp.data_offset < 5) {
        return false;
    }

    // Xử lý TCP Options (nếu có)
    size_t header_len_bytes = tcp.data_offset * 4;
    if (header_len_bytes > 20) {
        tcp.options_len = std::min((size_t)(header_len_bytes - 20), (size_t)tcp.options.size());
        memcpy(tcp.options.data(), data + 20, tcp.options_len);
    } else {
        tcp.options_len = 0;
    }

    return true;
}

void TCPParser::appendTreeView(std::string& tree, int depth, const TCPHeader& tcp) {
    appendTree(tree, depth, "Transmission Control Protocol");
    depth++;

    appendTree(tree, depth, "Source Port: " + getPortName(tcp.src_port));
    appendTree(tree, depth, "Destination Port: " + getPortName(tcp.dest_port));
    appendTree(tree, depth, "Sequence Number: " + std::to_string(tcp.seq_num));
    appendTree(tree, depth, "Acknowledgment Number: " + std::to_string(tcp.ack_num));
    appendTree(tree, depth, "Header Length: " + std::to_string(tcp.data_offset * 4) + " bytes");

    // Phân tích cờ (Flags)
    std::string flags_str;
    if (tcp.flags & TCPHeader::SYN) flags_str += "SYN, ";
    if (tcp.flags & TCPHeader::ACK) flags_str += "ACK, ";
    if (tcp.flags & TCPHeader::FIN) flags_str += "FIN, ";
    if (tcp.flags & TCPHeader::RST) flags_str += "RST, ";
    if (tcp.flags & TCPHeader::PSH) flags_str += "PSH, ";
    if (tcp.flags & TCPHeader::URG) flags_str += "URG, ";
    if (!flags_str.empty()) flags_str.pop_back(); // Xóa dấu phẩy

    appendTree(tree, depth, "Flags: " + to_hex(tcp.flags) + " (" + (flags_str.empty() ? "None" : flags_str) + ")");
    appendTree(tree, depth, "Window Size: " + std::to_string(tcp.window));
    appendTree(tree, depth, "Checksum: " + to_hex(tcp.checksum));
    appendTree(tree, depth, "Urgent Pointer: " + std::to_string(tcp.urgent_pointer));
}
