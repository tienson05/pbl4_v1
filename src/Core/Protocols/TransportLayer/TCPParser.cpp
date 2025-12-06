#include "TCPParser.hpp"
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cstring>
#include <sstream>
#include <iomanip>

#define TCPOPT_EOL 0
#define TCPOPT_NOP 1
#define TCPOPT_TIMESTAMP 8
// -----------------------------------------


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

void TCPParser::parseTCPOptions(TCPHeader& tcp, const uint8_t* options_data, size_t options_len)
{
    const uint8_t* ptr = options_data;
    size_t remaining = options_len;

    // (Reset cờ timestamp trước khi bắt đầu)
    tcp.has_timestamp = false;

    while (remaining > 0)
    {
        uint8_t kind = ptr[0]; // Lấy 'Kind'

        if (kind == TCPOPT_EOL) { // End of Option List
            return; // Kết thúc
        }

        if (kind == TCPOPT_NOP) { // No-Operation (Padding)
            ptr++;
            remaining--;
            continue; // Chuyển sang byte tiếp theo
        }

        // Nếu không phải EOL hay NOP, nó phải có 1 byte Length
        if (remaining < 2) return; // Lỗi (option bị cắt cụt)
        uint8_t len = ptr[1]; // Lấy 'Length'

        // Kiểm tra độ dài có hợp lệ không
        if (len < 2 || len > remaining) {
            return; // Lỗi (độ dài không hợp lệ)
        }

        // --- KIỂM TRA TIMESTAMP ---
        // (Kind = 8, Length = 10 (1 byte Kind + 1 byte Len + 4 byte TSval + 4 byte TSecr))
        if (kind == TCPOPT_TIMESTAMP && len == 10)
        {
            tcp.has_timestamp = true;

            // Trích xuất 4 byte TSval (Network Order)
            uint32_t ts_val_net;
            memcpy(&ts_val_net, ptr + 2, 4);
            tcp.ts_val = ntohl(ts_val_net); // Chuyển sang Host Order

            // Trích xuất 4 byte TSecr (Network Order)
            uint32_t ts_ecr_net;
            memcpy(&ts_ecr_net, ptr + 6, 4);
            tcp.ts_ecr = ntohl(ts_ecr_net); // Chuyển sang Host Order
        }

        // Chuyển sang Option tiếp theo
        ptr += len;
        remaining -= len;
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

    size_t header_len_bytes = tcp.data_offset * 4;

    // Kiểm tra xem header có bị cắt cụt không
    if (len < header_len_bytes) {
        return false; // Gói tin bị cắt
    }

    // Xử lý TCP Options (nếu có)
    if (header_len_bytes > 20) {
        tcp.options_len = (uint8_t)(header_len_bytes - 20);

        // --- GỌI HÀM PHÂN TÍCH
        parseTCPOptions(tcp, data + 20, tcp.options_len);

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

    // --- Hiển thị Timestamp trong Packet Details ---
    if (tcp.has_timestamp) {
        appendTree(tree, depth, "Options: (Timestamps)");
        appendTree(tree, depth + 1, "Timestamp value: " + std::to_string(tcp.ts_val));
        appendTree(tree, depth + 1, "Timestamp echo reply: " + std::to_string(tcp.ts_ecr));
    }
}
