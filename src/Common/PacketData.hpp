#ifndef PACKETDATA_HPP
#define PACKETDATA_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <ctime>
#include <net/ethernet.h>
#include <iostream>

// ==================== LAYER 2: ETHERNET ====================
struct EthernetHeader {
    std::array<uint8_t, 6> dest_mac{};
    std::array<uint8_t, 6> src_mac{};
    uint16_t ether_type = 0;      // 0x0800: IPv4, 0x86DD: IPv6, 0x0806: ARP, 0x8100: VLAN
};

// ==================== LAYER 2.5: VLAN (802.1Q) ====================
struct VLANHeader {
    uint16_t tpid = 0;          // 0x8100
    uint16_t tci = 0;           // Priority (3) + DEI (1) + VLAN ID (12)
    uint16_t ether_type = 0;    // Loại gói sau VLAN
};

// ==================== LAYER 3: IPv4 ====================
struct IPv4Header {
    uint8_t  version = 4;
    uint8_t  ihl = 0;
    uint8_t  tos = 0;
    uint16_t total_length = 0;
    uint16_t id = 0;
    uint16_t flags_frag_offset = 0;
    uint8_t  ttl = 0;
    uint8_t  protocol = 0;
    uint16_t header_checksum = 0;
    uint32_t src_ip = 0;
    uint32_t dest_ip = 0;
    std::array<uint8_t, 40> options{};
    uint8_t  options_len = 0;
};

// ==================== LAYER 3: IPv6 ====================
struct IPv6Header {
    uint32_t ver_tc_flow = 0;
    uint16_t payload_length = 0;
    uint8_t  next_header = 0;
    uint8_t  hop_limit = 0;
    std::array<uint8_t, 16> src_ip{};
    std::array<uint8_t, 16> dest_ip{};
};

// ==================== LAYER 3: ARP ====================
struct ARPHeader {
    uint16_t hardware_type = 0;
    uint16_t protocol_type = 0;
    uint8_t  hardware_size = 0;
    uint8_t  protocol_size = 0;
    uint16_t opcode = 0;
    std::array<uint8_t, 6> sender_mac{};
    uint32_t sender_ip = 0;
    std::array<uint8_t, 6> target_mac{};
    uint32_t target_ip = 0;
};

// ==================== LAYER 4: TCP ====================
struct TCPHeader {
    uint16_t src_port = 0;
    uint16_t dest_port = 0;
    uint32_t seq_num = 0;
    uint32_t ack_num = 0;
    uint8_t  data_offset = 0;
    uint8_t  reserved = 0;
    uint8_t  flags = 0;
    uint16_t window = 0;
    uint16_t checksum = 0;
    uint16_t urgent_pointer = 0;
    std::array<uint8_t, 40> options{};
    uint8_t  options_len = 0;

    // --- (SỬA ĐỔI) THÊM CÁC TRƯỜNG TIMESTAMP ---
    bool     has_timestamp = false;
    uint32_t ts_val = 0;
    uint32_t ts_ecr = 0;
    // ------------------------------------

    enum Flags : uint8_t {
        FIN = 0x01,
        SYN = 0x02,
        RST = 0x04,
        PSH = 0x08,
        ACK = 0x10,
        URG = 0x20,
        ECE = 0x40,
        CWR = 0x80
    };
};

// ==================== LAYER 4: UDP ====================
struct UDPHeader {
    uint16_t src_port = 0;
    uint16_t dest_port = 0;
    uint16_t length = 0;
    uint16_t checksum = 0;
};

// ==================== LAYER 4: ICMP ====================
struct ICMPHeader {
    uint8_t  type = 0;
    uint8_t  code = 0;
    uint16_t checksum = 0;
    uint16_t id = 0;
    uint16_t sequence = 0;
};

// ==================== APPLICATION LAYER ====================
struct ApplicationLayer {
    std::vector<uint8_t> data;
    std::string protocol;      // HTTP, DNS, TLS, ...
    std::string info;

    // HTTP
    std::string http_method;
    std::string http_host;
    std::string http_path;
    std::string http_version;
    bool is_http_request = false;
    bool is_http_response = false;
    int  http_status_code = 0;

    // DNS
    uint16_t dns_id = 0;
    bool is_dns_query = false;
    std::string dns_name;
    uint16_t dns_type = 0;
    uint16_t dns_class = 0;

    // TLS
    uint8_t tls_version_major = 0;
    uint8_t tls_version_minor = 0;
    std::string tls_sni;
};

// ==================== CẤU TRÚC CHÍNH: PacketData ====================
struct PacketData {
    // Metadata
    uint32_t packet_id = 0;
    timespec timestamp{};
    uint32_t cap_length = 0;
    uint32_t wire_length = 0;

    // Raw Data
    std::vector<uint8_t> raw_packet;

    // Layer 2
    EthernetHeader eth{};
    VLANHeader vlan{};
    bool has_vlan = false;

    // Layer 3
    IPv4Header ipv4{};
    IPv6Header ipv6{};
    ARPHeader  arp{};
    bool is_ipv4 = false;
    bool is_ipv6 = false;
    bool is_arp  = false;

    // Layer 4
    TCPHeader tcp{};
    UDPHeader udp{};
    ICMPHeader icmp{};
    bool is_tcp = false;
    bool is_udp = false;
    bool is_icmp = false;

    // Application
    ApplicationLayer app{};

    // Tree view (Wireshark style)
    std::string tree_view;
    int tree_depth = 0;

    // Expert info
    std::string expert_info;
    bool is_malformed = false;
    bool is_retransmitted = false;
    bool is_duplicate = false;

    // ======= Methods =======

    /**
     * @brief (ĐÃ SỬA) Reset toàn bộ dữ liệu của struct
     * để chuẩn bị phân tích gói tin mới.
     */
    void clear(){
        raw_packet.clear();
        tree_view.clear();
        expert_info.clear();
        tree_depth = 0;

        has_vlan = is_ipv4 = is_ipv6 = is_arp = false;
        is_tcp = is_udp = is_icmp = false;
        is_malformed = is_retransmitted = is_duplicate = false;

        eth = EthernetHeader{};
        vlan = VLANHeader{};
        ipv4 = IPv4Header{};
        ipv6 = IPv6Header{};
        arp = ARPHeader{};
        tcp = TCPHeader{};
        udp = UDPHeader{};
        icmp = ICMPHeader{};
        app = ApplicationLayer{};
    }

    std::string toJson() const;
    void printTree() const{
        std::cout << tree_view;
    }
};

// (Cần thiết để QVariant lưu trữ PacketData)
#include <QVariant>
Q_DECLARE_METATYPE(PacketData)

#endif // PACKETDATA_HPP
