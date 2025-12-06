#include "IPv6Parser.hpp"
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
// --- Các hàm trợ giúp nội bộ ---

static void appendTree(std::string& tree, int depth, const std::string& line) {
    tree += std::string(depth * 2, ' ') + line + "\n";
}

static std::string ipv6ToString(const std::array<uint8_t, 16>& ip) {
    char buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, ip.data(), buf, sizeof(buf));
    return std::string(buf);
}

static std::string to_string_8(uint8_t val) {
    return std::to_string(val);
}

// --- Triển khai (Implementation) ---


bool IPv6Parser::parse(IPv6Header& ipv6, const uint8_t*& data, size_t& len) {
    // Header IPv6 cố định là 40 bytes
    if (len < 40) {
        return false;
    }

    const struct ip6_hdr* ip6_hdr = (const struct ip6_hdr*)data;

    ipv6.ver_tc_flow   = ntohl(ip6_hdr->ip6_flow);
    ipv6.payload_length = ntohs(ip6_hdr->ip6_plen);
    ipv6.next_header   = ip6_hdr->ip6_nxt; // Đây là Next Header *đầu tiên*
    ipv6.hop_limit     = ip6_hdr->ip6_hlim;

    memcpy(ipv6.src_ip.data(), &ip6_hdr->ip6_src, 16);
    memcpy(ipv6.dest_ip.data(), &ip6_hdr->ip6_dst, 16);

    if (((ipv6.ver_tc_flow >> 28) & 0x0F) != 6) {
        return false; // Không phải là IPv6
    }

    data += 40; // Tăng con trỏ qua header chính
    len -= 40;

    uint8_t current_next_header = ipv6.next_header;
    bool parsing_extensions = true;

    while (parsing_extensions) {
        switch (current_next_header) {

        // Các Extension Header cần bỏ qua
        case IPPROTO_HOPOPTS:  // Hop-by-Hop (0)
        case IPPROTO_ROUTING:  // Routing (43)
        case IPPROTO_DSTOPTS:  // Destination Options (60)
        {
            if (len < 8) return false; // Ít nhất 8 byte cho ext header
            const struct ip6_ext* ext_hdr = (const struct ip6_ext*)data;
            current_next_header = ext_hdr->ip6e_nxt; // Chuyển sang header tiếp theo

            // Độ dài của header này (tính bằng 8-byte, *không* bao gồm 8 byte đầu)
            size_t ext_len = (ext_hdr->ip6e_len + 1) * 8;

            if (len < ext_len) return false; // Gói tin bị cắt
            data += ext_len;
            len -= ext_len;
            break;
        }

        // Header Phân mảnh (Fragment)
        case IPPROTO_FRAGMENT: // (44)
        {
            if (len < 8) return false; // Header Fragment cố định 8 byte
            const struct ip6_frag* frag_hdr = (const struct ip6_frag*)data;
            current_next_header = frag_hdr->ip6f_nxt; // Chuyển sang header tiếp theo

            data += 8;
            len -= 8;
            break;
        }

        // Đây là các header Tầng 4 (hoặc không phải extension)
        // Chúng ta dừng vòng lặp
        case IPPROTO_TCP:    // 6
        case IPPROTO_UDP:    // 17
        case IPPROTO_ICMPV6: // 58
        case IPPROTO_NONE:   // 59 (Không có header tiếp theo)
        default:             // Bất kỳ protocol nào khác (ví dụ: ESP, AH)
            parsing_extensions = false;
            break;
        }
    }


    // Cập nhật 'next_header' thành protocol Tầng 4 thực sự
    ipv6.next_header = current_next_header;

    return true;
}

void IPv6Parser::appendTreeView(std::string& tree, int depth, const IPv6Header& ipv6) {
    appendTree(tree, depth, "Internet Protocol Version 6");
    depth++;

    uint8_t  version = (ipv6.ver_tc_flow >> 28) & 0x0F;
    uint8_t  traffic_class = (ipv6.ver_tc_flow >> 20) & 0xFF;
    uint32_t flow_label = ipv6.ver_tc_flow & 0x000FFFFF;

    appendTree(tree, depth, "Version: " + to_string_8(version));
    appendTree(tree, depth, "Traffic Class: 0x" + to_string_8(traffic_class));
    appendTree(tree, depth, "Flow Label: 0x" + std::to_string(flow_label));
    appendTree(tree, depth, "Payload Length: " + std::to_string(ipv6.payload_length));

    // (Next Header này là protocol Tầng 4 (sau khi đã skip))
    appendTree(tree, depth, "Next Header: " + to_string_8(ipv6.next_header));

    appendTree(tree, depth, "Hop Limit: " + to_string_8(ipv6.hop_limit));
    appendTree(tree, depth, "Source: " + ipv6ToString(ipv6.src_ip));
    appendTree(tree, depth, "Destination: " + ipv6ToString(ipv6.dest_ip));
}
