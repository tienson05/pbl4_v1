#ifndef VLANPARSER_HPP
#define VLANPARSER_HPP

#include "../../../Common/PacketData.hpp"
#include "EthernetParser.hpp"
#include <cstring>
#include <arpa/inet.h>
#include <iomanip>
#include <sstream>

class VLANParser {
public:
    static bool parse(VLANHeader& vlan, const uint8_t* data, size_t len, uint16_t& next_ether_type) {
        if (len < 4) return false;

        vlan.tpid = ntohs(*reinterpret_cast<const uint16_t*>(data));
        vlan.tci  = ntohs(*reinterpret_cast<const uint16_t*>(data + 2));

        if (vlan.tpid != 0x8100 && vlan.tpid != 0x88A8 && vlan.tpid != 0x9100) {
            return false;               // Không phải VLAN tag hợp lệ
        }

        if (len >= 6) {
            next_ether_type = ntohs(*reinterpret_cast<const uint16_t*>(data + 4));
            vlan.ether_type = next_ether_type;
        } else {
            vlan.ether_type = 0;
        }
        return true;
    }

    static std::string getPriority(uint16_t tci) {
        return std::to_string((tci >> 13) & 0x07);
    }

    static std::string getDEI(uint16_t tci) {
        return ((tci >> 12) & 0x01) ? "Drop Eligible" : "Not Drop Eligible";
    }

    static std::string getVID(uint16_t tci) {
        uint16_t vid = tci & 0xFFF;
        if (vid == 0)      return "0 (Priority tag)";
        if (vid == 0xFFF)  return "4095 (Reserved)";
        return std::to_string(vid);
    }

    static std::string tciToString(uint16_t tci) {
        std::ostringstream oss;
        oss << "Priority: " << getPriority(tci)
            << ", DEI: " << ((tci >> 12) & 1)
            << ", VID: " << getVID(tci);
        return oss.str();
    }

    static std::string tpidToString(uint16_t tpid) {
        switch (tpid) {
        case 0x8100: return "802.1Q";
        case 0x88A8: return "802.1ad (Q-in-Q)";
        case 0x9100: return "Legacy VLAN";
        default:     return "Unknown VLAN Tag";
        }
    }

    // -----------------------------------------------------------------
    // Helper: chuyển số thành chuỗi hex có độ rộng cố định
    // -----------------------------------------------------------------
    static std::string to_hex(uint16_t val, int width = 4) {
        std::ostringstream oss;
        oss << std::hex << std::setw(width) << std::setfill('0') << val;
        return oss.str();
    }

    // -----------------------------------------------------------------
    // appendTreeView – đã được sửa lỗi nối chuỗi
    // -----------------------------------------------------------------
    static void appendTreeView(std::string& tree, int depth, const VLANHeader& vlan) {
        const std::string indent(depth * 2, ' ');

        // 802.1Q Virtual LAN, <type>
        tree += indent + "802.1Q Virtual LAN, " + tpidToString(vlan.tpid) + "\n";

        // TPID: 0x....
        tree += indent + "  Tag Protocol Identifier (TPID): 0x" + to_hex(vlan.tpid) + "\n";

        // TCI: Priority:..., DEI:..., VID:...
        tree += indent + "  Tag Control Information (TCI): " + tciToString(vlan.tci) + "\n";

        // Encapsulated Protocol: <type> (0x....)
        tree += indent + "  Encapsulated Protocol: "
                + EthernetParser::etherTypeToString(vlan.ether_type)
                + " (0x" + to_hex(vlan.ether_type) + ")\n";
    }
};

#endif // VLANPARSER_HPP
