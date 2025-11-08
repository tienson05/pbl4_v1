#ifndef IPV4PARSER_HPP
#define IPV4PARSER_HPP

#include "../../../Common/PacketData.hpp"
#include <arpa/inet.h>
#include <iomanip>
#include <sstream>
#include <algorithm>

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

        ipv4.tos             = data[1];
        ipv4.total_length    = ntohs(*reinterpret_cast<const uint16_t*>(data + 2));
        ipv4.id              = ntohs(*reinterpret_cast<const uint16_t*>(data + 4));
        ipv4.flags_frag_offset = ntohs(*reinterpret_cast<const uint16_t*>(data + 6));
        ipv4.ttl             = data[8];
        ipv4.protocol        = data[9];
        ipv4.header_checksum = ntohs(*reinterpret_cast<const uint16_t*>(data + 10));
        ipv4.src_ip          = *reinterpret_cast<const uint32_t*>(data + 12);
        ipv4.dest_ip         = *reinterpret_cast<const uint32_t*>(data + 16);

        ipv4.options_len = header_len > 20 ? header_len - 20 : 0;
        if (ipv4.options_len > 0) {
            std::copy_n(data + 20,
                        std::min(ipv4.options_len, static_cast<uint8_t>(40)),
                        ipv4.options.begin());
        }

        return true;
    }

    static std::string ipToString(uint32_t ip) {
        std::ostringstream oss;
        oss << ((ip >> 24) & 0xFF) << "."
            << ((ip >> 16) & 0xFF) << "."
            << ((ip >>  8) & 0xFF) << "."
            << ( ip        & 0xFF);
        return oss.str();
    }

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

        // TOS (DSCP + ECN)
        tree += indent + "  Differentiated Services: 0x"
                + to_hex(ipv4.tos, 2) + "\n";

        tree += indent + "  Total Length: " + std::to_string(ipv4.total_length) + " bytes\n";

        // Identification
        tree += indent + "  Identification: 0x" + to_hex(ipv4.id)
                + " (" + std::to_string(ipv4.id) + ")\n";

        // Flags & Fragment Offset
        uint16_t flags = ipv4.flags_frag_offset >> 13;
        uint16_t frag_offset = ipv4.flags_frag_offset & 0x1FFF;
        tree += indent + "  Flags: 0x" + to_hex(flags, 1)
                + " (" + flagsToString(ipv4.flags_frag_offset) + ")\n";
        tree += indent + "  Fragment Offset: " + std::to_string(frag_offset) + "\n";

        tree += indent + "  Time to Live: " + std::to_string(ipv4.ttl) + "\n";
        tree += indent + "  Protocol: " + protocolToString(ipv4.protocol)
                + " (" + std::to_string(static_cast<int>(ipv4.protocol)) + ")\n";

        // Header Checksum
        tree += indent + "  Header Checksum: 0x" + to_hex(ipv4.header_checksum) + "\n";

        tree += indent + "  Source: " + ipToString(ipv4.src_ip) + "\n";
        tree += indent + "  Destination: " + ipToString(ipv4.dest_ip) + "\n";

        if (ipv4.options_len > 0) {
            tree += indent + "  Options: " + std::to_string(ipv4.options_len) + " bytes\n";
        }
    }
};

#endif // IPV4PARSER_HPP
