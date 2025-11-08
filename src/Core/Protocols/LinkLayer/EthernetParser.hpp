#ifndef ETHERNETPARSER_HPP
#define ETHERNETPARSER_HPP

#include "../../../Common/PacketData.hpp"
#include <cstring>
#include <arpa/inet.h>
#include <iomanip>
#include <sstream>
#include <algorithm>

class EthernetParser {
public:
    static bool parse(EthernetHeader& eth, const uint8_t* data, size_t len, bool& has_vlan, VLANHeader& vlan) {
        if (len < sizeof(struct ether_header)) {
            return false;
        }

        const struct ether_header* eth_hdr = reinterpret_cast<const struct ether_header*>(data);

        std::copy_n(eth_hdr->ether_dhost, 6, eth.dest_mac.begin());
        std::copy_n(eth_hdr->ether_shost, 6, eth.src_mac.begin());
        eth.ether_type = ntohs(eth_hdr->ether_type);

        const uint8_t* payload = data + sizeof(struct ether_header);
        size_t payload_len = len - sizeof(struct ether_header);

        has_vlan = false;

        if (eth.ether_type == 0x8100 && payload_len >= 4) {
            has_vlan = true;
            vlan.tpid = eth.ether_type;
            vlan.tci = ntohs(*reinterpret_cast<const uint16_t*>(payload));
            vlan.ether_type = ntohs(*reinterpret_cast<const uint16_t*>(payload + 2));

            eth.vlan_tpid = vlan.tpid;
            eth.vlan_tci = vlan.tci;
            eth.ether_type = vlan.ether_type;

            return true;
        }

        return true;
    }

    static std::string macToString(const std::array<uint8_t, 6>& mac) {
        std::ostringstream oss;
        for (size_t i = 0; i < 6; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(mac[i]);
            if (i < 5) oss << ":";
        }
        return oss.str();
    }

    static std::string etherTypeToString(uint16_t type) {
        switch (type) {
        case 0x0800: return "IPv4";
        case 0x86DD: return "IPv6";
        case 0x0806: return "ARP";
        case 0x8100: return "VLAN (802.1Q)";
        case 0x88A8: return "802.1ad (Q-in-Q)";
        case 0x9100: return "Legacy VLAN";
        case 0x8864: return "PPPoE Session";
        case 0x8863: return "PPPoE Discovery";
        default: {
            std::ostringstream oss;
            oss << "0x" << std::hex << std::setw(4) << std::setfill('0') << type;
            return oss.str();
        }
        }
    }

    static std::string vlanTciToString(uint16_t tci) {
        uint8_t pcp = (tci >> 13) & 0x07;
        bool dei = (tci >> 12) & 0x01;
        uint16_t vid = tci & 0xFFF;

        std::ostringstream oss;
        oss << "PCP=" << static_cast<int>(pcp)
            << ", DEI=" << (dei ? 1 : 0)
            << ", VID=" << vid;
        return oss.str();
    }

    static void appendTreeView(std::string& tree, int depth, const EthernetHeader& eth, bool has_vlan, const VLANHeader& vlan) {
        std::string indent(depth * 2, ' ');

        tree += indent + "Ethernet II\n";
        tree += indent + "  Destination: " + macToString(eth.dest_mac) + "\n";
        tree += indent + "  Source: " + macToString(eth.src_mac) + "\n";

        // EtherType hex
        std::ostringstream type_oss;
        type_oss << std::hex << std::setw(4) << std::setfill('0') << eth.ether_type;
        tree += indent + "  Type: " + etherTypeToString(eth.ether_type) + " (0x" + type_oss.str() + ")\n";

        if (has_vlan) {
            tree += indent + "802.1Q Virtual LAN\n";

            // TPID hex
            std::ostringstream tpid_oss;
            tpid_oss << std::hex << std::setw(4) << std::setfill('0') << vlan.tpid;
            tree += indent + "  TPID: 0x" + tpid_oss.str() + "\n";

            tree += indent + "  TCI: " + vlanTciToString(vlan.tci) + "\n";

            // VLAN EtherType hex
            std::ostringstream vlan_type_oss;
            vlan_type_oss << std::hex << std::setw(4) << std::setfill('0') << vlan.ether_type;
            tree += indent + "  Type: " + etherTypeToString(vlan.ether_type) + " (0x" + vlan_type_oss.str() + ")\n";
        }
    }
};

#endif // ETHERNETPARSER_HPP
