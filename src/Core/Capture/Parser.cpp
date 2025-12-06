#include "../../Common/PacketData.hpp"
#include "Parser.hpp"

#include "../Protocols/LinkLayer/EthernetParser.hpp"
#include "../Protocols/LinkLayer/VLANParser.hpp"
#include "../Protocols/NetworkLayer/IPv4Parser.hpp"
#include "../Protocols/NetworkLayer/IPv6Parser.hpp"
#include "../Protocols/NetworkLayer/ARPParser.hpp"
#include "../Protocols/NetworkLayer/ICMPParser.hpp"
#include "../Protocols/TransportLayer/TCPParser.hpp"
#include "../Protocols/TransportLayer/UDPParser.hpp"
#include "../Protocols/ApplicationLayer/ApplicationParser.hpp"

#include <cstring>
#include <ctime>
#include <arpa/inet.h>



void Parser::appendTree(PacketData* pkt, const std::string& line) {
    pkt->tree_view += std::string(pkt->tree_depth * 2, ' ') + line + "\n";
}

bool Parser::parse(PacketData* pkt, const uint8_t* data, size_t len) {
    if (!pkt || !data || len == 0) return false;

    pkt->clear();
    pkt->raw_packet.assign(data, data + len);
    pkt->cap_length = pkt->wire_length = len;
    clock_gettime(CLOCK_REALTIME, &pkt->timestamp);

    const uint8_t* ptr = data;
    size_t remaining = len;
    pkt->tree_depth = 0;
    pkt->tree_view.clear();

    // ==================== ETHERNET ====================
    if (!EthernetParser::parse(pkt->eth, ptr, remaining, pkt->has_vlan, pkt->vlan)) {
        pkt->is_malformed = true;
        pkt->tree_view = "[Malformed Ethernet Header]\n";
        return false;
    }

    ptr += 14; remaining -= 14;
    EthernetParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->eth, pkt->has_vlan, pkt->vlan);

    uint16_t next_proto = pkt->has_vlan ? pkt->vlan.ether_type : pkt->eth.ether_type;

    // ==================== VLAN ====================
    if (pkt->has_vlan) {
        if (remaining < 4) {
            pkt->is_malformed = true;
            appendTree(pkt, "[Truncated VLAN Tag]");
            return false;
        }
        ptr += 4; remaining -= 4;
        VLANParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->vlan);
        next_proto = pkt->vlan.ether_type;
    }

    // ==================== LAYER 3 ====================
    if (next_proto == 0x0800 && remaining >= 20) {
        // --- IPv4 ---
        pkt->is_ipv4 = IPv4Parser::parse(pkt->ipv4, ptr, remaining);
        if (!pkt->is_ipv4) {
            appendTree(pkt, "[Malformed IPv4 Header]");
            pkt->is_malformed = true;
            return false;
        }

        size_t ip_hdr_len = pkt->ipv4.ihl * 4;
        ptr += ip_hdr_len;
        remaining -= ip_hdr_len;
        IPv4Parser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->ipv4);

        // --- Layer 4 (cho IPv4) ---
        uint8_t proto = pkt->ipv4.protocol;
        if (proto == 6 && remaining >= 20) { // TCP
            pkt->is_tcp = TCPParser::parse(pkt->tcp, ptr, remaining);
            if (pkt->is_tcp) {
                size_t tcp_hdr_len = pkt->tcp.data_offset * 4;
                ptr += tcp_hdr_len; remaining -= tcp_hdr_len;
                TCPParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->tcp);
ApplicationParser::parse(pkt->app, ptr, remaining, pkt->tcp.src_port, pkt->tcp.dest_port, true);            }
        }
        else if (proto == 17 && remaining >= 8) { // UDP
            pkt->is_udp = UDPParser::parse(pkt->udp, ptr, remaining);
            if (pkt->is_udp) {
                ptr += 8; remaining -= 8;
                UDPParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->udp);
ApplicationParser::parse(pkt->app, ptr, remaining, pkt->udp.src_port, pkt->udp.dest_port, false);            }
        }
        else if (proto == 1 && remaining >= 4) { // ICMPv4
            pkt->is_icmp = ICMPParser::parse(pkt->icmp, ptr, remaining);
            if (pkt->is_icmp) {
                ICMPParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->icmp);
            }
        }
    }

    // --- KHỐI IPv6 ---
    else if (next_proto == 0x86DD && remaining >= 40) {
        // --- IPv6 ---
        // 'ptr' và 'remaining' sẽ bị thay đổi bởi hàm parse()
        pkt->is_ipv6 = IPv6Parser::parse(pkt->ipv6, ptr, remaining);

        if (pkt->is_ipv6) {
            IPv6Parser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->ipv6);

            //  Layer 4 (cho IPv6) ---
            uint8_t proto = pkt->ipv6.next_header;

            if (proto == 6 && remaining >= 20) { // TCP
                pkt->is_tcp = TCPParser::parse(pkt->tcp, ptr, remaining);
                if (pkt->is_tcp) {
                    size_t tcp_hdr_len = pkt->tcp.data_offset * 4;
                    ptr += tcp_hdr_len; remaining -= tcp_hdr_len;
                    TCPParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->tcp);
                     // --- KÍCH HOẠT TẦNG 7 ---
ApplicationParser::parse(pkt->app, ptr, remaining, pkt->tcp.src_port, pkt->tcp.dest_port, true);                }
            }
            else if (proto == 17 && remaining >= 8) { // UDP
                pkt->is_udp = UDPParser::parse(pkt->udp, ptr, remaining);
                if (pkt->is_udp) {
                    ptr += 8; remaining -= 8;
                    UDPParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->udp);
                     // --- KÍCH HOẠT TẦNG 7 ---
ApplicationParser::parse(pkt->app, ptr, remaining, pkt->udp.src_port, pkt->udp.dest_port, false);                }
            }
            else if (proto == 58 && remaining >= 4) { // 58 = ICMPv6
                pkt->is_icmp = ICMPParser::parse(pkt->icmp, ptr, remaining);
                if (pkt->is_icmp) {
                    ICMPParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->icmp);
                }
            }
        }
    }

    else if (next_proto == 0x0806 && remaining >= 28) { // <-- ARP
        pkt->is_arp = ARPParser::parse(pkt->arp, ptr, remaining);
        if (pkt->is_arp) {
            ARPParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->arp);
        }
    }
    else { // <-- Unknown
        appendTree(pkt, "[Unknown EtherType: " + EthernetParser::to_hex(next_proto) + "]");
    }

    // ==================== APPLICATION ====================
    if (!pkt->app.protocol.empty()) {
        ApplicationParser::appendTreeView(pkt->tree_view, pkt->tree_depth++, pkt->app);
    }

    return true;
}
