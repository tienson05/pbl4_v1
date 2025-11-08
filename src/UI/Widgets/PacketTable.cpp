// PacketTable.cpp
#include "PacketTable.hpp"
#include <QVBoxLayout>
#include <QSplitter>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QHeaderView>
#include <QScrollBar>
#include <QVariant>
#include <QDateTime>
#include <ctime>
#include <iomanip>
#include <sstream>

PacketTable::PacketTable(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    // Kết nối sự kiện click dòng
    connect(packetList, &QTableWidget::itemClicked, this, &PacketTable::onPacketRowSelected);
}

void PacketTable::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // === Bảng gói tin ===
    packetList = new QTableWidget(this);
    packetList->setColumnCount(7); // 6 cột hiển thị + 1 cột ẩn
    QStringList headers = {"No.", "Time", "Source", "Destination", "Protocol", "Info"};
    packetList->setHorizontalHeaderLabels(headers);
    packetList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    packetList->setSelectionBehavior(QAbstractItemView::SelectRows);
    packetList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    packetList->hideColumn(6); // Ẩn cột Length (nếu không dùng)

    // === Chi tiết gói tin (Tree) ===
    packetDetails = new QTreeWidget(this);
    packetDetails->setHeaderLabel("Packet Details");
    packetDetails->setColumnCount(1);
    packetDetails->setIndentation(15);

    // === Hex dump ===
    packetBytes = new QTextEdit(this);
    packetBytes->setFont(QFont("Courier", 9));
    packetBytes->setReadOnly(true);

    // === Splitter ===
    QSplitter *bottomSplitter = new QSplitter(Qt::Horizontal, this);
    bottomSplitter->addWidget(packetDetails);
    bottomSplitter->addWidget(packetBytes);
    bottomSplitter->setSizes({400, 300});

    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->addWidget(packetList);
    mainSplitter->addWidget(bottomSplitter);
    mainSplitter->setSizes({300, 400});

    layout->addWidget(mainSplitter);
    setLayout(layout);
}

// === THÊM GÓI TIN ===
void PacketTable::addPacket(const PacketData &packet)
{
    int row = packetList->rowCount();
    packetList->insertRow(row);

    // CỘT 0: No.
    packetList->setItem(row, 0, new QTableWidgetItem(QString::number(packet.packet_id)));

    // CỘT 1: Time
    char timeStr[64];
    struct tm *tm_info = localtime(&packet.timestamp.tv_sec);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);
    QString time = QString("%1.%2")
                       .arg(timeStr)
                       .arg(packet.timestamp.tv_nsec / 1000000, 3, 10, QChar('0'));
    packetList->setItem(row, 1, new QTableWidgetItem(time));

    // CỘT 2: Source
    QString src = getSourceAddress(packet);
    packetList->setItem(row, 2, new QTableWidgetItem(src));

    // CỘT 3: Destination
    QString dst = getDestinationAddress(packet);
    packetList->setItem(row, 3, new QTableWidgetItem(dst));

    // CỘT 4: Protocol
    QString proto = getProtocolName(packet);
    packetList->setItem(row, 4, new QTableWidgetItem(proto));

    // CỘT 5: Info
    QString info = getInfo(packet);
    packetList->setItem(row, 5, new QTableWidgetItem(info));

    // CỘT ẨN (6): Lưu toàn bộ PacketData
    QTableWidgetItem* hidden = new QTableWidgetItem();
    hidden->setData(Qt::UserRole, QVariant::fromValue(packet));
    hidden->setFlags(Qt::NoItemFlags);
    packetList->setItem(row, 6, hidden);

    // Scroll xuống
    packetList->scrollToBottom();
}

// === KHI CLICK DÒNG ===
void PacketTable::onPacketRowSelected(QTableWidgetItem *item)
{
    int row = item->row();
    QTableWidgetItem *hidden = packetList->item(row, 6);
    if (!hidden) return;

    PacketData packet = hidden->data(Qt::UserRole).value<PacketData>();
    showPacketDetails(packet);
    showHexDump(packet);
}

// === HIỂN THỊ CHI TIẾT 4 TẦNG ===
void PacketTable::showPacketDetails(const PacketData &packet)
{
    packetDetails->clear();

    QTreeWidgetItem *root = new QTreeWidgetItem(packetDetails);
    root->setText(0, QString("Frame %1: %2 bytes")
                         .arg(packet.packet_id)
                         .arg(packet.wire_length));
    root->setExpanded(true);

    // Ethernet
    QTreeWidgetItem *eth = new QTreeWidgetItem(root);
    eth->setText(0, "Ethernet II");
    addField(eth, "Destination", macToString(packet.eth.dest_mac));
    addField(eth, "Source", macToString(packet.eth.src_mac));
    addField(eth, "Type", QString("0x%1 (%2)")
                              .arg(packet.eth.ether_type, 4, 16, QChar('0')).toUpper()
                              .arg(getEtherTypeName(packet.eth.ether_type)));

    if (packet.has_vlan) {
        QTreeWidgetItem *vlan = new QTreeWidgetItem(root);
        vlan->setText(0, "802.1Q Virtual LAN");
        addField(vlan, "TPID", QString("0x%1").arg(packet.vlan.tpid, 4, 16, QChar('0')));
        addField(vlan, "Priority", QString::number((packet.vlan.tci >> 13) & 0x7));
        addField(vlan, "DEI", QString::number((packet.vlan.tci >> 12) & 0x1));
        addField(vlan, "VLAN ID", QString::number(packet.vlan.tci & 0xFFF));
    }

    // IP / ARP
    if (packet.is_ipv4) {
        QTreeWidgetItem *ip = new QTreeWidgetItem(root);
        ip->setText(0, "Internet Protocol Version 4");
        addField(ip, "Source", ipToString(packet.ipv4.src_ip));
        addField(ip, "Destination", ipToString(packet.ipv4.dest_ip));
        addField(ip, "Protocol", QString("%1 (%2)").arg(packet.ipv4.protocol).arg(getIpProtoName(packet.ipv4.protocol)));
        addField(ip, "TTL", QString::number(packet.ipv4.ttl));
        addField(ip, "Header Length", QString("%1 bytes").arg(packet.ipv4.ihl * 4));
    }
    else if (packet.is_ipv6) {
        QTreeWidgetItem *ip = new QTreeWidgetItem(root);
        ip->setText(0, "Internet Protocol Version 6");
        addField(ip, "Source", ipv6ToString(packet.ipv6.src_ip));
        addField(ip, "Destination", ipv6ToString(packet.ipv6.dest_ip));
        addField(ip, "Next Header", QString::number(packet.ipv6.next_header));
        addField(ip, "Hop Limit", QString::number(packet.ipv6.hop_limit));
    }
    else if (packet.is_arp) {
        QTreeWidgetItem *arp = new QTreeWidgetItem(root);
        arp->setText(0, "Address Resolution Protocol");
        addField(arp, "Sender MAC", macToString(packet.arp.sender_mac));
        addField(arp, "Sender IP", ipToString(packet.arp.sender_ip));
        addField(arp, "Target MAC", macToString(packet.arp.target_mac));
        addField(arp, "Target IP", ipToString(packet.arp.target_ip));
        addField(arp, "Opcode", packet.arp.opcode == 1 ? "Request" : "Reply");
    }

    // TCP / UDP
    if (packet.is_tcp) {
        QTreeWidgetItem *tcp = new QTreeWidgetItem(root);
        tcp->setText(0, "Transmission Control Protocol");
        addField(tcp, "Source Port", QString::number(packet.tcp.src_port));
        addField(tcp, "Destination Port", QString::number(packet.tcp.dest_port));
        QString flags;
        if (packet.tcp.flags & TCPHeader::SYN) flags += "SYN ";
        if (packet.tcp.flags & TCPHeader::ACK) flags += "ACK ";
        if (packet.tcp.flags & TCPHeader::FIN) flags += "FIN ";
        if (packet.tcp.flags & TCPHeader::RST) flags += "RST ";
        if (packet.tcp.flags & TCPHeader::PSH) flags += "PSH ";
        addField(tcp, "Flags", flags.trimmed().isEmpty() ? "None" : flags.trimmed());
        addField(tcp, "Sequence Number", QString::number(packet.tcp.seq_num));
        addField(tcp, "Window Size", QString::number(packet.tcp.window));
    }
    else if (packet.is_udp) {
        QTreeWidgetItem *udp = new QTreeWidgetItem(root);
        udp->setText(0, "User Datagram Protocol");
        addField(udp, "Source Port", QString::number(packet.udp.src_port));
        addField(udp, "Destination Port", QString::number(packet.udp.dest_port));
        addField(udp, "Length", QString::number(packet.udp.length));
    }

    // Application
    if (!packet.app.protocol.empty()) {
        QTreeWidgetItem *app = new QTreeWidgetItem(root);
        app->setText(0, QString::fromStdString("Application: " + packet.app.protocol));
        if (packet.app.is_http_request) {
            addField(app, "Request", QString::fromStdString(packet.app.http_method + " " + packet.app.http_path));
            if (!packet.app.http_host.empty())
                addField(app, "Host", QString::fromStdString(packet.app.http_host));
        }
        else if (packet.app.is_http_response) {
            addField(app, "Response", QString("HTTP/%1 %2")
                         .arg(QString::fromStdString(packet.app.http_version))
                         .arg(packet.app.http_status_code));
        }
        else if (packet.app.is_dns_query) {
            addField(app, "Query", QString::fromStdString(packet.app.dns_name));
            addField(app, "Type", QString::number(packet.app.dns_type));
        }
    }

    packetDetails->expandAll();
}

// === HEX DUMP ===
void PacketTable::showHexDump(const PacketData &packet)
{
    std::ostringstream oss;
    const auto& data = packet.raw_packet;
    size_t len = data.size();

    for (size_t i = 0; i < len; i += 16) {
        // Offset
        oss << QString("%1").arg(i, 8, 16, QChar('0')).toUpper().toStdString() << "  ";

        // Hex
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < len)
                oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i + j] << " ";
            else
                oss << "   ";
            if (j == 7) oss << " ";
        }

        // ASCII
        oss << " ";
        for (size_t j = 0; j < 16 && i + j < len; ++j) {
            char c = data[i + j];
            oss << (c >= 32 && c <= 126 ? c : '.');
        }
        oss << "\n";
    }

    packetBytes->setText(QString::fromStdString(oss.str()));
}

// === HÀM HỖ TRỢ ===
QString PacketTable::getSourceAddress(const PacketData &p)
{
    if (p.is_arp) return macToString(p.arp.sender_mac);
    if (p.is_ipv4) {
        QString s = ipToString(p.ipv4.src_ip);
        if (p.is_tcp) s += ":" + QString::number(p.tcp.src_port);
        else if (p.is_udp) s += ":" + QString::number(p.udp.src_port);
        return s;
    }
    if (p.is_ipv6) {
        QString s = ipv6ToString(p.ipv6.src_ip);
        if (p.is_tcp) s += ":" + QString::number(p.tcp.src_port);
        else if (p.is_udp) s += ":" + QString::number(p.udp.src_port);
        return s;
    }
    return macToString(p.eth.src_mac);
}

QString PacketTable::getDestinationAddress(const PacketData &p)
{
    if (p.is_arp) return macToString(p.arp.target_mac);
    if (p.is_ipv4) {
        QString s = ipToString(p.ipv4.dest_ip);
        if (p.is_tcp) s += ":" + QString::number(p.tcp.dest_port);
        else if (p.is_udp) s += ":" + QString::number(p.udp.dest_port);
        return s;
    }
    if (p.is_ipv6) {
        QString s = ipv6ToString(p.ipv6.dest_ip);
        if (p.is_tcp) s += ":" + QString::number(p.tcp.dest_port);
        else if (p.is_udp) s += ":" + QString::number(p.udp.dest_port);
        return s;
    }
    return macToString(p.eth.dest_mac);
}

QString PacketTable::getProtocolName(const PacketData &p)
{
    if (p.is_arp) return "ARP";
    if (p.is_icmp) return "ICMP";
    if (p.is_tcp) return "TCP";
    if (p.is_udp) return "UDP";
    if (!p.app.protocol.empty()) return QString::fromStdString(p.app.protocol);
    return QString("0x%1").arg(p.eth.ether_type, 4, 16, QChar('0')).toUpper();
}

QString PacketTable::getInfo(const PacketData &p)
{
    if (p.is_tcp) {
        QString f;
        if (p.tcp.flags & TCPHeader::SYN) f += "SYN ";
        if (p.tcp.flags & TCPHeader::ACK) f += "ACK ";
        if (p.tcp.flags & TCPHeader::FIN) f += "FIN ";
        if (p.tcp.flags & TCPHeader::RST) f += "RST ";
        return f.trimmed().isEmpty() ? "[TCP]" : f.trimmed();
    }
    if (p.is_udp && p.app.protocol == "DNS") {
        return p.app.is_dns_query ? "DNS Query" : "DNS Response";
    }
    if (p.app.is_http_request) {
        return QString::fromStdString(p.app.http_method + " " + p.app.http_path);
    }
    if (p.app.is_http_response) {
        return QString("HTTP %1").arg(p.app.http_status_code);
    }
    if (!p.app.info.empty()) {
        return QString::fromStdString(p.app.info);
    }
    return QString("Len=%1").arg(p.cap_length);
}

void PacketTable::addField(QTreeWidgetItem *parent, const QString &name, const QString &value)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, name + ": " + value);
}

QString PacketTable::macToString(const std::array<uint8_t, 6>& mac)
{
    return QString("%1:%2:%3:%4:%5:%6")
    .arg(mac[0], 2, 16, QChar('0'))
        .arg(mac[1], 2, 16, QChar('0'))
        .arg(mac[2], 2, 16, QChar('0'))
        .arg(mac[3], 2, 16, QChar('0'))
        .arg(mac[4], 2, 16, QChar('0'))
        .arg(mac[5], 2, 16, QChar('0')).toUpper();
}

QString PacketTable::ipToString(uint32_t ip)
{
    return QString("%1.%2.%3.%4")
    .arg((ip >> 24) & 0xFF)
        .arg((ip >> 16) & 0xFF)
        .arg((ip >> 8)  & 0xFF)
        .arg(ip & 0xFF);
}

QString PacketTable::ipv6ToString(const std::array<uint8_t, 16>& ip)
{
    QStringList parts;
    for (int i = 0; i < 16; i += 2) {
        uint16_t part = (ip[i] << 8) | ip[i + 1];
        parts << QString("%1").arg(part, 0, 16);
    }
    return parts.join(":");
}

QString PacketTable::getEtherTypeName(uint16_t type)
{
    switch (type) {
    case 0x0800: return "IPv4";
    case 0x86DD: return "IPv6";
    case 0x0806: return "ARP";
    case 0x8100: return "VLAN";
    default: return "Unknown";
    }
}

QString PacketTable::getIpProtoName(uint8_t proto)
{
    switch (proto) {
    case 1: return "ICMP";
    case 6: return "TCP";
    case 17: return "UDP";
    default: return "Unknown";
    }
}
