#ifndef IPV4PARSER_HPP
#define IPV4PARSER_HPP

#include "../../../Common/PacketData.hpp"
#include <arpa/inet.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstring> // <-- THÊM MỚI: Cần cho memcpy

class IPv4Parser {
public:
    // -----------------------------------------------------------------
    // Helper: chuyển số thành chuỗi hex có độ rộng cố định
    // -----------------------------------------------------------------
    static std::string to_hex(uint16_t val, int width = 4) {
        std::ostringstream oss;
        oss << std::hex << std::setw(width) << std::setfill('0') << val;
        return oss.str();
    }

    static bool parse(IPv4Header& ipv4, const uint8_t* data, size_t len) {
        if (len < 20) return false;

        ipv4.version = data[0] >> 4;
        ipv4.ihl     = data[0] & 0x0F;
        if (ipv4.version != 4 || ipv4.ihl < 5) return false;

        size_t header_len = ipv4.ihl * 4;
        if (len < header_len) return false;

        ipv4.tos               = data[1];
        ipv4.total_length      = ntohs(*reinterpret_cast<const uint16_t*>(data + 2));
        ipv4.id                = ntohs(*reinterpret_cast<const uint16_t*>(data + 4));
        ipv4.flags_frag_offset = ntohs(*reinterpret_cast<const uint16_t*>(data + 6));
        ipv4.ttl               = data[8];
        ipv4.protocol          = data[9];
        ipv4.header_checksum   = ntohs(*reinterpret_cast<const uint16_t*>(data + 10));

        // --- ĐÂY LÀ PHẦN SỬA LỖI ---
        // Chúng ta phải chuyển đổi từ Network Order (Big Endian)
        // sang Host Order (Little Endian)

        // 1. Sao chép (copy) 4 byte (Network Order) vào một biến tạm
        uint32_t src_ip_net, dest_ip_net;
        memcpy(&src_ip_net, data + 12, 4);
        memcpy(&dest_ip_net, data + 16, 4);

        // 2. Dùng ntohl() để chuyển đổi và lưu trữ (ở dạng Host Order)
        ipv4.src_ip  = ntohl(src_ip_net);
        ipv4.dest_ip = ntohl(dest_ip_net);
        // --- KẾT THÚC SỬA LỖI ---


        ipv4.options_len = header_len > 20 ? header_len - 20 : 0;
        if (ipv4.options_len > 0) {
            std::copy_n(data + 20,
                        std::min(ipv4.options_len, static_cast<uint8_t>(40)),
                        ipv4.options.begin());
        }

        return true;
    }

    // --- HÀM ipToString ĐÃ BỊ XÓA ---
    // (Vì hàm này thuộc về PacketTable.cpp)

    static std::string protocolToString(uint8_t proto) {
        switch (proto) {
        case 1:   return "ICMP";
        case 2:   return "IGMP";
        case 6:   return "TCP";
        case 17:  return "UDP";
        case 89:  return "OSPF";
        case 132: return "SCTP";
        default:  return std::to_string(proto);
        }
    }

    static std::string flagsToString(uint16_t flags) {
        std::string s;
        if (flags & 0x4000) s += "Reserved, ";
        if (flags & 0x2000) s += "Don't Fragment, ";
        if (flags & 0x1000) s += "More Fragments, ";
        if (!s.empty()) s.resize(s.size() - 2);
        return s.empty() ? "0" : s;
    }

    static void appendTreeView(std::string& tree, int depth, const IPv4Header& ipv4) {
        const std::string indent(depth * 2, ' ');

        tree += indent + "Internet Protocol Version 4 (IPv4)\n";
        tree += indent + "  Version: " + std::to_string(ipv4.version) + "\n";
        tree += indent + "  Header Length: " + std::to_string(ipv4.ihl * 4) + " bytes (" + std::to_string(ipv4.ihl) + ")\n";

        tree += indent + "  Differentiated Services: 0x"
                + to_hex(ipv4.tos, 2) + "\n";

        tree += indent + "  Total Length: " + std::to_string(ipv4.total_length) + " bytes\n";

        tree += indent + "  Identification: 0x" + to_hex(ipv4.id)
                + " (" + std::to_string(ipv4.id) + ")\n";

        uint16_t flags = ipv4.flags_frag_offset >> 13;
        uint16_t frag_offset = ipv4.flags_frag_offset & 0x1FFF;
        tree += indent + "  Flags: 0x" + to_hex(flags, 1)
                + " (" + flagsToString(ipv4.flags_frag_offset) + ")\n";
        tree += indent + "  Fragment Offset: " + std::to_string(frag_offset) + "\n";

        tree += indent + "  Time to Live: " + std::to_string(ipv4.ttl) + "\n";
        tree += indent + "  Protocol: " + protocolToString(ipv4.protocol)
                + " (" + std::to_string(static_cast<int>(ipv4.protocol)) + ")\n";

        tree += indent + "  Header Checksum: 0x" + to_hex(ipv4.header_checksum) + "\n";

        // (Xóa 2 dòng hiển thị IP ở đây, vì PacketTable sẽ lo việc đó)
        tree += indent + "  Source: (IP được lưu ở dạng Host Order)\n";
        tree += indent + "  Destination: (IP được lưu ở dạng Host Order)\n";

        if (ipv4.options_len > 0) {
            tree += indent + "  Options: " + std::to_string(ipv4.options_len) + " bytes\n";
        }
    }
};

#endif // IPV4PARSER_HPP
