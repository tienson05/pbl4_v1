// PacketFormatter.cpp
#include "PacketFormatter.hpp"
#include "../../Core/Protocols/NetworkLayer/ICMPParser.hpp"
#include "../../Common/MacResolver.hpp"
#include <QDateTime>
#include <QRegularExpression>

// === 1. LOGIC PHÂN TÍCH GÓI TIN VÀO CÂY (Main Logic) ===
void PacketFormatter::populateTree(QTreeWidget* tree, const PacketData& packet)
{
    tree->clear();

    int eth_offset = 0;
    int eth_len = 14;
    int l3_offset = eth_offset + 14;
    int l4_offset = -1;
    int l7_offset = -1;

    // --- TẦNG 1: FRAME ---
    QTreeWidgetItem *root = new QTreeWidgetItem(tree);
    root->setText(0, QString("Frame %1: %2 bytes on wire (%3 bits), %4 bytes captured (%5 bits)")
                         .arg(packet.packet_id).arg(packet.wire_length).arg(packet.wire_length * 8)
                         .arg(packet.cap_length).arg(packet.cap_length * 8));
    // Metadata cho hex highlight
    root->setData(0, Qt::UserRole + 1, 0);
    root->setData(0, Qt::UserRole + 2, packet.cap_length);

    QDateTime timestamp = QDateTime::fromSecsSinceEpoch(packet.timestamp.tv_sec);
    timestamp = timestamp.addMSecs(packet.timestamp.tv_nsec / 1000000);
    addField(root, "Arrival Time", timestamp.toLocalTime().toString("MMM d, yyyy hh:mm:ss.zzz"));
    addField(root, "Frame Number", QString::number(packet.packet_id));
    addField(root, "Frame Length", QString("%1 bytes").arg(packet.wire_length));
    addField(root, "Capture Length", QString("%1 bytes").arg(packet.cap_length));

    // --- TẦNG 2: ETHERNET ---
    QTreeWidgetItem *eth = new QTreeWidgetItem(root);
    eth->setText(0, "Ethernet II");
    eth->setData(0, Qt::UserRole + 1, eth_offset);
    eth->setData(0, Qt::UserRole + 2, eth_len);

    QString destMacRaw = macToString(packet.eth.dest_mac);
    QString srcMacRaw = macToString(packet.eth.src_mac);
    QString destLabel = QString("%1 (%2)").arg(getResolvedMacLabel(destMacRaw)).arg(destMacRaw);
    QString srcLabel = QString("%1 (%2)").arg(getResolvedMacLabel(srcMacRaw)).arg(srcMacRaw);

    addField(eth, "Destination", destLabel, eth_offset + 0, 6);
    addField(eth, "Source", srcLabel, eth_offset + 6, 6);
    addField(eth, "Type", QString("%1 (%2)").arg(toHex(packet.eth.ether_type)).arg(getEtherTypeName(packet.eth.ether_type)), eth_offset + 12, 2);

    if (packet.has_vlan) {
        l3_offset += 4;
        QTreeWidgetItem *vlan = new QTreeWidgetItem(root);
        vlan->setText(0, "802.1Q Virtual LAN");
        addField(vlan, "TPID", toHex(packet.vlan.tpid), eth_offset + 14, 2);
        addField(vlan, "TCI", toHex(packet.vlan.tci), eth_offset + 16, 2);
        addField(vlan, "Type", QString("%1 (%2)").arg(toHex(packet.vlan.ether_type)).arg(getEtherTypeName(packet.vlan.ether_type)), eth_offset + 18, 2);
    }

    // --- TẦNG 3: IP/ARP ---
    if (packet.is_ipv4) {
        int ip_hdr_len = packet.ipv4.ihl * 4;
        l4_offset = l3_offset + ip_hdr_len;
        QTreeWidgetItem *ip = new QTreeWidgetItem(root);
        ip->setText(0, "Internet Protocol Version 4");
        ip->setData(0, Qt::UserRole + 1, l3_offset);
        ip->setData(0, Qt::UserRole + 2, ip_hdr_len);

        addField(ip, "Version/IHL", toHex(packet.ipv4.version << 4 | packet.ipv4.ihl, 2), l3_offset + 0, 1);
        addField(ip, "Header Length", QString("%1 bytes").arg(ip_hdr_len));
        addField(ip, "Total Length", QString::number(packet.ipv4.total_length), l3_offset + 2, 2);
        addField(ip, "TTL", QString::number(packet.ipv4.ttl), l3_offset + 8, 1);
        addField(ip, "Protocol", QString("%1 (%2)").arg(getIpProtoName(packet.ipv4.protocol)).arg(packet.ipv4.protocol), l3_offset + 9, 1);
        addField(ip, "Source Address", ipToString(packet.ipv4.src_ip), l3_offset + 12, 4);
        addField(ip, "Destination Address", ipToString(packet.ipv4.dest_ip), l3_offset + 16, 4);
    }
    else if (packet.is_ipv6) {
        l4_offset = l3_offset + 40; // IPv6 header cố định 40 bytes
        QTreeWidgetItem *ip = new QTreeWidgetItem(root);
        ip->setText(0, "Internet Protocol Version 6");
        ip->setData(0, Qt::UserRole + 1, l3_offset);
        ip->setData(0, Qt::UserRole + 2, 40);
        addField(ip, "Source", ipv6ToString(packet.ipv6.src_ip), l3_offset + 8, 16);
        addField(ip, "Destination", ipv6ToString(packet.ipv6.dest_ip), l3_offset + 24, 16);
    }
    else if (packet.is_arp) {
        QTreeWidgetItem *arp = new QTreeWidgetItem(root);
        arp->setText(0, "Address Resolution Protocol");
        arp->setData(0, Qt::UserRole + 1, l3_offset);
        arp->setData(0, Qt::UserRole + 2, 28);

        QString op = (packet.arp.opcode == 1) ? "Request (1)" : "Reply (2)";
        addField(arp, "Opcode", op, l3_offset + 6, 2);

        QString sMac = macToString(packet.arp.sender_mac);
        QString tMac = macToString(packet.arp.target_mac);
        addField(arp, "Sender MAC", QString("%1 (%2)").arg(getResolvedMacLabel(sMac)).arg(sMac), l3_offset + 8, 6);
        addField(arp, "Sender IP", ipToString(packet.arp.sender_ip), l3_offset + 14, 4);
        addField(arp, "Target MAC", QString("%1 (%2)").arg(getResolvedMacLabel(tMac)).arg(tMac), l3_offset + 18, 6);
        addField(arp, "Target IP", ipToString(packet.arp.target_ip), l3_offset + 24, 4);
    }

    // --- TẦNG 4: TCP/UDP/ICMP ---
    if (l4_offset != -1) {
        if (packet.is_tcp) {
            int tcp_hdr_len = packet.tcp.data_offset * 4;
            l7_offset = l4_offset + tcp_hdr_len;
            QTreeWidgetItem *tcp = new QTreeWidgetItem(root);
            tcp->setText(0, "Transmission Control Protocol");
            tcp->setData(0, Qt::UserRole + 1, l4_offset);
            tcp->setData(0, Qt::UserRole + 2, tcp_hdr_len);

            addField(tcp, "Source Port", QString::number(packet.tcp.src_port), l4_offset + 0, 2);
            addField(tcp, "Destination Port", QString::number(packet.tcp.dest_port), l4_offset + 2, 2);
            addField(tcp, "Seq", QString::number(packet.tcp.seq_num), l4_offset + 4, 4);
            addField(tcp, "Ack", QString::number(packet.tcp.ack_num), l4_offset + 8, 4);
            addField(tcp, "Flags", toHex(packet.tcp.flags, 2), l4_offset + 13, 1);
            if (packet.tcp.options_len > 0) {
                addField(tcp, "Options", QString("%1 bytes").arg(packet.tcp.options_len), l4_offset + 20, packet.tcp.options_len);
            }
        }
        else if (packet.is_udp) {
            l7_offset = l4_offset + 8;
            QTreeWidgetItem *udp = new QTreeWidgetItem(root);
            udp->setText(0, "User Datagram Protocol");
            udp->setData(0, Qt::UserRole + 1, l4_offset);
            udp->setData(0, Qt::UserRole + 2, 8);
            addField(udp, "Source Port", QString::number(packet.udp.src_port), l4_offset + 0, 2);
            addField(udp, "Destination Port", QString::number(packet.udp.dest_port), l4_offset + 2, 2);
            addField(udp, "Length", QString::number(packet.udp.length), l4_offset + 4, 2);
        }
        else if (packet.is_icmp) {
            int icmp_len = packet.cap_length - l4_offset;
            l7_offset = l4_offset + 8;
            QTreeWidgetItem *icmp = new QTreeWidgetItem(root);
            icmp->setText(0, "Internet Control Message Protocol");
            icmp->setData(0, Qt::UserRole + 1, l4_offset);
            icmp->setData(0, Qt::UserRole + 2, icmp_len);

            QString typeStr = QString::fromStdString(ICMPParser::getTypeString(packet.icmp.type));
            addField(icmp, "Type", QString("%1 (%2)").arg(packet.icmp.type).arg(typeStr), l4_offset + 0, 1);
            addField(icmp, "Code", QString::number(packet.icmp.code), l4_offset + 1, 1);
        }
    }

    // --- TẦNG 7: APP ---
    if (l7_offset != -1 && l7_offset < packet.cap_length) {
        int app_len = packet.cap_length - l7_offset;
        QTreeWidgetItem *app = new QTreeWidgetItem(root);
        app->setText(0, QString::fromStdString("Application: " + packet.app.protocol));
        app->setData(0, Qt::UserRole + 1, l7_offset);
        app->setData(0, Qt::UserRole + 2, app_len);

        addField(app, "Data Length", QString("%1 bytes").arg(app_len), l7_offset, app_len);
        if (!packet.app.info.empty()) {
            addField(app, "Info", QString::fromStdString(packet.app.info));
        }
    }

    tree->expandAll();
}

// === 2. LOGIC HEX DUMP ===
void PacketFormatter::displayHexDump(QTextEdit* textEdit, const PacketData& packet, int hl_offset, int hl_len)
{
    const auto& data = packet.raw_packet;
    size_t len = data.size();

    // Set Font
    QFont font("Courier", 9);
    textEdit->setFont(font);

    std::stringstream oss;
    oss << "<html><head><style>"
        << "body { font-family: 'Courier'; font-size: 9pt; white-space: pre; }"
        << ".hl { background-color: #a0c0f0; color: black; }" // Màu highlight
        << "</style></head><body>";

    for (size_t i = 0; i < len; i += 16) {
        // Offset cột trái
        oss << "<b>" << QString("%1").arg(i, 8, 16, QChar('0')).toUpper().toStdString() << "</b>  ";

        // Hex part
        for (size_t j = 0; j < 16; ++j) {
            size_t idx = i + j;
            if (idx < len) {
                bool highlight = (hl_offset != -1) && (idx >= (size_t)hl_offset) && (idx < (size_t)(hl_offset + hl_len));
                if (highlight) oss << "<span class=\"hl\">";
                oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[idx] << " ";
                if (highlight) oss << "</span>";
            } else {
                oss << "   ";
            }
            if (j == 7) oss << " ";
        }

        // ASCII part
        oss << " ";
        for (size_t j = 0; j < 16 && (i + j) < len; ++j) {
            size_t idx = i + j;
            char c = data[idx];
            bool highlight = (hl_offset != -1) && (idx >= (size_t)hl_offset) && (idx < (size_t)(hl_offset + hl_len));

            if (highlight) oss << "<span class=\"hl\">";

            if (c >= 32 && c <= 126) {
                if (c == '<') oss << "&lt;";
                else if (c == '>') oss << "&gt;";
                else if (c == '&') oss << "&amp;";
                else oss << c;
            } else {
                oss << ".";
            }

            if (highlight) oss << "</span>";
        }
        oss << "<br>";
    }
    oss << "</body></html>";
    textEdit->setHtml(QString::fromStdString(oss.str()));
}

// === 3. CÁC HÀM GET INFO KHÁC ===

QColor PacketFormatter::getRowColor(const QString& proto) {
    if (proto == "TCP" || proto == "HTTP") return QColor(230, 245, 225);
    if (proto == "TLS") return QColor(230, 225, 245);
    if (proto == "UDP" || proto == "DNS" || proto == "QUIC" || proto == "MDNS" || proto == "SSDP") {
        return QColor(225, 240, 255); // Màu xanh dương nhạt
    }
    if (proto == "ICMP") return QColor(255, 245, 220);
    if (proto == "ARP") return QColor(250, 240, 250);
    return Qt::white;
}

QString PacketFormatter::formatTime(const struct timespec& ts) {
    char timeStr[64];
    struct tm *tm_info = localtime(&ts.tv_sec);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);

    // timespec dùng tv_nsec (nanosecond), chia 1.000.000 để ra millisecond
    return QString("%1.%2").arg(timeStr).arg(ts.tv_nsec / 1000000, 3, 10, QChar('0'));
}

QString PacketFormatter::getProtocolName(const PacketData& p) {
    if (!p.app.protocol.empty()) return QString::fromStdString(p.app.protocol);
    if (p.is_tcp) return "TCP";
    if (p.is_udp) return "UDP";
    if (p.is_icmp) return "ICMP";
    if (p.is_arp) return "ARP";
    return QString("0x%1").arg(p.eth.ether_type, 4, 16, QChar('0')).toUpper();
}

QString PacketFormatter::getSource(const PacketData& p) {
    if (p.is_arp) return getResolvedMacLabel(macToString(p.eth.src_mac));
    if (p.is_ipv4) return ipToString(p.ipv4.src_ip);
    if (p.is_ipv6) return ipv6ToString(p.ipv6.src_ip);
    return getResolvedMacLabel(macToString(p.eth.src_mac));
}

QString PacketFormatter::getDest(const PacketData& p) {
    if(p.is_arp) return getResolvedMacLabel(macToString(p.eth.dest_mac));
    if (p.is_ipv4) return ipToString(p.ipv4.dest_ip);
    if (p.is_ipv6) return ipv6ToString(p.ipv6.dest_ip);
    return getResolvedMacLabel(macToString(p.eth.dest_mac));
}

QString PacketFormatter::getInfo(const PacketData& p) {
    if (!p.app.info.empty()) return QString::fromStdString(p.app.info);
    if (p.app.is_http_request) return QString::fromStdString(p.app.http_method + " " + p.app.http_path);
    if (p.app.is_http_response) return QString("HTTP %1").arg(p.app.http_status_code);

    if (p.is_tcp) {
        QString info = QString("%1 -> %2 Seq=%3 Ack=%4 Win=%5")
        .arg(p.tcp.src_port).arg(p.tcp.dest_port)
            .arg(p.tcp.seq_num).arg(p.tcp.ack_num).arg(p.tcp.window);
        // Có thể thêm Flags vào đây nếu muốn
        return info;
    }
    if (p.is_udp) return QString("%1 -> %2 Len=%3").arg(p.udp.src_port).arg(p.udp.dest_port).arg(p.udp.length);
    if (p.is_arp) return (p.arp.opcode == 1) ? "Who has?" : "Is at";
    if (p.is_icmp) return QString::fromStdString(ICMPParser::getTypeString(p.icmp.type));

    return QString("Len=%1").arg(p.cap_length);
}

// === INTERNAL HELPERS ===

void PacketFormatter::addField(QTreeWidgetItem *parent, const QString &name, const QString &value, int offset, int length) {
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, name + ": " + value);
    if (offset != -1) {
        item->setData(0, Qt::UserRole + 1, offset);
        item->setData(0, Qt::UserRole + 2, length);
    }
}

QString PacketFormatter::getResolvedMacLabel(const QString& rawMac) {
    std::string vendor = MacResolver::instance().getVendor(rawMac.toStdString());
    if (vendor == "Unknown") return rawMac;
    return QString::fromStdString(vendor) + "_" + rawMac.right(8);
}

QString PacketFormatter::macToString(const std::array<uint8_t, 6>& mac) {
    return QString("%1:%2:%3:%4:%5:%6")
    .arg(mac[0], 2, 16, QChar('0')).arg(mac[1], 2, 16, QChar('0'))
        .arg(mac[2], 2, 16, QChar('0')).arg(mac[3], 2, 16, QChar('0'))
        .arg(mac[4], 2, 16, QChar('0')).arg(mac[5], 2, 16, QChar('0')).toUpper();
}

QString PacketFormatter::ipToString(uint32_t ip) {
    return QString("%1.%2.%3.%4").arg(ip & 0xFF).arg((ip >> 8) & 0xFF).arg((ip >> 16) & 0xFF).arg((ip >> 24) & 0xFF);
}

QString PacketFormatter::ipv6ToString(const std::array<uint8_t, 16>& ip) {
    QStringList parts;
    for (int i = 0; i < 16; i += 2) {
        parts << QString("%1").arg((ip[i] << 8) | ip[i+1], 0, 16);
    }
    return parts.join(":");
}

QString PacketFormatter::getEtherTypeName(uint16_t type) {
    switch (type) {
    case 0x0800: return "IPv4";
    case 0x86DD: return "IPv6";
    case 0x0806: return "ARP";
    case 0x8100: return "VLAN";
    default: return "Unknown";
    }
}

QString PacketFormatter::getIpProtoName(uint8_t proto) {
    switch (proto) {
    case 1: return "ICMP";
    case 6: return "TCP";
    case 17: return "UDP";
    default: return "Unknown";
    }
}
