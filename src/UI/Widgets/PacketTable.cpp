// PacketTable.cpp
#include "PacketTable.hpp"
#include "../../Core/Protocols/NetworkLayer/ICMPParser.hpp" // (Sửa thành ../../)
#include <QVBoxLayout>
#include <QSplitter>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QHeaderView>
#include <QScrollBar>
#include <QVariant>
#include <QDateTime>
#include <QColor>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <QRegularExpression>
#include <QTimer> // <-- THÊM MỚI

// === (MỚI) CÁC THAM SỐ TỐI ƯU HÓA ===
const int CHUNK_SIZE = 500; // Xử lý 500 gói mỗi lần
const int TIMER_INTERVAL_MS = 30; // ~33 FPS

PacketTable::PacketTable(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    // Kết nối sự kiện click dòng
    connect(packetList, &QTableWidget::itemClicked, this, &PacketTable::onPacketRowSelected);

    // --- (MỚI) THIẾT LẬP BỘ ĐỆM CHỐNG LAG ---
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(TIMER_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &PacketTable::processPacketChunk);
    m_updateTimer->start();

    // (MỚI) Theo dõi thanh cuộn để tự động cuộn
    connect(packetList->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value){
        QScrollBar* vbar = packetList->verticalScrollBar();
        m_isUserAtBottom = (vbar == nullptr) || (value >= vbar->maximum() - 5);
    });
}

void PacketTable::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    packetList = new QTableWidget(this);

    // (Code này đã đúng từ file của bạn)
    packetList->setColumnCount(8);
    QStringList headers = {"No.", "Time", "Source", "Destination", "Protocol", "Length", "Info"};
    packetList->setHorizontalHeaderLabels(headers);
    packetList->horizontalHeader()->setStretchLastSection(true);
    packetList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    packetList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    packetList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    packetList->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    packetList->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    packetList->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    packetList->setSelectionBehavior(QAbstractItemView::SelectRows);
    packetList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    packetList->hideColumn(7);
    packetList->verticalHeader()->setVisible(false);
    packetDetails = new QTreeWidget(this);
    packetDetails->setHeaderLabel("Packet Details");
    packetDetails->setColumnCount(1);
    packetDetails->setIndentation(15);
    packetBytes = new QTextEdit(this);
    packetBytes->setFont(QFont("Courier", 9));
    packetBytes->setReadOnly(true);
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

void PacketTable::clearData()
{
    packetList->setRowCount(0);
    packetDetails->clear();
    packetBytes->clear();
    m_packetBuffer.clear(); // <-- (MỚI) Xóa luôn hàng đợi
}

/**
 * @brief (ĐÃ SỬA) Slot nhận MỘT gói tin
 * Chỉ thêm vào hàng đợi. QTimer sẽ xử lý.
 */
void PacketTable::onPacketReceived(const PacketData &packet)
{
    m_packetBuffer.append(packet);
}

/**
 * @brief (ĐÃ SỬA) Slot nhận MỘT LÔ (batch) gói tin
 * Dùng std::move để "cướp" dữ liệu (siêu nhanh O(1)).
 */
void PacketTable::onPacketsReceived(QList<PacketData>* packets)
{
    // Nối list của con trỏ vào m_packetBuffer
    m_packetBuffer.append(std::move(*packets));

    // 'packets' bây giờ là một list rỗng, chúng ta xóa nó
    delete packets;
}

/**
 * @brief (MỚI) Được gọi bởi QTimer, xử lý một phần nhỏ của buffer
 */
void PacketTable::processPacketChunk()
{
    if (m_packetBuffer.isEmpty()) {
        return; // Không có gì để làm
    }

    // --- TỐI ƯU HÓA (OPTIMIZATION) ---
    packetList->setUpdatesEnabled(false);
    packetList->setSortingEnabled(false);
    // ----------------------------------

    // Sửa lỗi build: ép kiểu .size() về int
    int packetsToProcess = qMin(CHUNK_SIZE, (int)m_packetBuffer.size());

    for (int i = 0; i < packetsToProcess; ++i) {
        // Lấy gói tin ra khỏi ĐẦU hàng đợi
        PacketData packet = m_packetBuffer.takeFirst();

        // Gọi hàm helper để thêm hàng
        insertPacketRow(packet);
    }

    // --- BẬT LẠI TÍNH NĂNG VẼ ---
    packetList->setSortingEnabled(true);
    packetList->setUpdatesEnabled(true);
    // ----------------------------------

    if (m_isUserAtBottom) {
        packetList->scrollToBottom();
    }
}

/**
 * @brief (MỚI) Hàm helper để chèn 1 hàng vào bảng.
 * (Đây chính là code 'addPacket' cũ của bạn)
 */
void PacketTable::insertPacketRow(const PacketData &packet)
{
    int row = packetList->rowCount();
    packetList->insertRow(row);

    QTableWidgetItem* noItem = new QTableWidgetItem(QString::number(packet.packet_id));
    char timeStr[64];
    struct tm *tm_info = localtime(&packet.timestamp.tv_sec);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);
    QString time = QString("%1.%2")
                       .arg(timeStr)
                       .arg(packet.timestamp.tv_nsec / 1000000, 3, 10, QChar('0'));
    QString proto = getProtocolName(packet);
    QTableWidgetItem* lengthItem = new QTableWidgetItem(QString::number(packet.wire_length));

    QList<QTableWidgetItem*> items = {
        noItem,                                 // 0
        new QTableWidgetItem(time),                 // 1
        new QTableWidgetItem(getSourceAddress(packet)),   // 2
        new QTableWidgetItem(getDestinationAddress(packet)),// 3
        new QTableWidgetItem(proto),                // 4
        lengthItem,                                 // 5
        new QTableWidgetItem(getInfo(packet))       // 6
    };

    QColor color;
    if (proto == "TCP" || proto == "HTTP") {
        color.setRgb(230, 245, 225); // Xanh lá nhạt
    } else if (proto == "TLS") {
        color.setRgb(230, 225, 245); // Tím nhạt
    } else if (proto == "QUIC") { // <-- THÊM MỚI
        color.setRgb(235, 235, 235); // Xám nhạt (giống Wireshark)
    } else if (proto == "UDP" || proto == "DNS") {
        color.setRgb(225, 240, 255); // Xanh dương nhạt
    } else if (proto == "ICMP") {
        color.setRgb(255, 245, 220); // Vàng/Cam nhạt
    } else if (proto == "ARP") {
        color.setRgb(250, 240, 250); // Hồng nhạt
    } else {
        color = Qt::white; // Mặc định
    }

    for(int i = 0; i < items.size(); ++i) {
        items[i]->setBackground(color);
        packetList->setItem(row, i, items[i]);
    }

    QTableWidgetItem* hidden = new QTableWidgetItem();
    hidden->setData(Qt::UserRole, QVariant::fromValue(packet));
    hidden->setFlags(Qt::NoItemFlags);
    packetList->setItem(row, 7, hidden);
}


// === KHI CLICK DÒNG ===
void PacketTable::onPacketRowSelected(QTableWidgetItem *item)
{
    int row = item->row();
    QTableWidgetItem *hidden = packetList->item(row, 7); // (Đã đúng: Cột 7)
    if (!hidden) return;
    PacketData packet = hidden->data(Qt::UserRole).value<PacketData>();
    showPacketDetails(packet);
    showHexDump(packet);
}

// === HIỂN THỊ CHI TIẾT 4 TẦNG ===
void PacketTable::showPacketDetails(const PacketData &packet)
{
    // (Code này đã đúng từ file của bạn,
    //  vì nó đã chứa logic hiển thị TSval)
    packetDetails->clear();
    QTreeWidgetItem *root = new QTreeWidgetItem(packetDetails);
    root->setText(0, QString("Frame %1: %2 bytes")
                         .arg(packet.packet_id)
                         .arg(packet.wire_length));
    root->setExpanded(true);

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

        if (packet.tcp.has_timestamp) {
            QTreeWidgetItem *ts = new QTreeWidgetItem(tcp);
            ts->setText(0, "Options: (Timestamps)");
            addField(ts, "Timestamp value", QString::number(packet.tcp.ts_val));
            addField(ts, "Timestamp echo reply", QString::number(packet.tcp.ts_ecr));
        }
    }
    else if (packet.is_udp) {
        QTreeWidgetItem *udp = new QTreeWidgetItem(root);
        udp->setText(0, "User Datagram Protocol");
        addField(udp, "Source Port", QString::number(packet.udp.src_port));
        addField(udp, "Destination Port", QString::number(packet.udp.dest_port));
        addField(udp, "Length", QString::number(packet.udp.length));
    }
    else if (packet.is_icmp) {
        QTreeWidgetItem *icmp = new QTreeWidgetItem(root);
        icmp->setText(0, "Internet Control Message Protocol");

        // (Sử dụng hàm static mới từ ICMPParser)
        QString typeStr = QString::fromStdString(ICMPParser::getTypeString(packet.icmp.type));
        addField(icmp, "Type", QString("%1 (%2)").arg(packet.icmp.type).arg(typeStr));
        addField(icmp, "Code", QString::number(packet.icmp.code));
        addField(icmp, "Checksum", QString("0x%1").arg(packet.icmp.checksum, 4, 16, QChar('0')));

        // (Chỉ hiển thị ID/Seq nếu là Echo)
        if (packet.icmp.type == 0 || packet.icmp.type == 8) {
            addField(icmp, "Identifier (id)", QString("0x%1 (%2)").arg(packet.icmp.id, 4, 16, QChar('0')).arg(packet.icmp.id));
            addField(icmp, "Sequence Number (seq)", QString("0x%1 (%2)").arg(packet.icmp.sequence, 4, 16, QChar('0')).arg(packet.icmp.sequence));
        }
    }

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
        oss << QString("%1").arg(i, 8, 16, QChar('0')).toUpper().toStdString() << "  ";
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < len)
                oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i + j] << " ";
            else
                oss << "   ";
            if (j == 7) oss << " ";
        }
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
    if (p.is_arp) return macToString(p.eth.src_mac);
    if (p.is_ipv4) return ipToString(p.ipv4.src_ip);
    if (p.is_ipv6) return ipv6ToString(p.ipv6.src_ip);
    return macToString(p.eth.src_mac);
}

QString PacketTable::getDestinationAddress(const PacketData &p)
{
    if(p.is_arp) return macToString(p.eth.dest_mac);
    if (p.is_ipv4) return ipToString(p.ipv4.dest_ip);
    if (p.is_ipv6) return ipv6ToString(p.ipv6.dest_ip);
    return macToString(p.eth.dest_mac);
}

QString PacketTable::getProtocolName(const PacketData &p)
{
    if (!p.app.protocol.empty()) {
        return QString::fromStdString(p.app.protocol);
    }
    if (p.is_tcp) return "TCP";
    if (p.is_udp) return "UDP";
    if (p.is_icmp) return "ICMP";
    if (p.is_arp) return "ARP";
    return QString("0x%1").arg(p.eth.ether_type, 4, 16, QChar('0')).toUpper();
}

QString PacketTable::getInfo(const PacketData &p)
{
    if (!p.app.info.empty()) {
        return QString::fromStdString(p.app.info);
    }

    if (p.app.is_http_request) {
        return QString::fromStdString(p.app.http_method + " " + p.app.http_path);
    }
    if (p.app.is_http_response) {
        return QString("HTTP %1").arg(p.app.http_status_code);
    }
    if (p.is_udp && p.app.protocol == "DNS") {
        return p.app.is_dns_query ? "DNS Query" : "DNS Response";
    }

    if (p.is_tcp) {
        QString info = QString("%1 → %2 ")
                           .arg(p.tcp.src_port)
                           .arg(p.tcp.dest_port);
        QString f;
        if (p.tcp.flags & TCPHeader::SYN) f += "SYN, ";
        if (p.tcp.flags & TCPHeader::ACK) f += "ACK, ";
        if (p.tcp.flags & TCPHeader::FIN) f += "FIN, ";
        if (p.tcp.flags & TCPHeader::RST) f += "RST, ";
        if (p.tcp.flags & TCPHeader::PSH) f += "PSH, ";
        if (!f.isEmpty()) {
            f.chop(2);
            info += QString("[%1] ").arg(f);
        }
        info += QString("Seq=%1 Ack=%2 Win=%3")
                    .arg(p.tcp.seq_num)
                    .arg(p.tcp.ack_num)
                    .arg(p.tcp.window);

        int payload_len = 0;
        if (p.is_ipv4) {
            int ip_total_len = p.ipv4.total_length;
            int ip_header_len = p.ipv4.ihl * 4;
            int tcp_header_len = p.tcp.data_offset * 4;
            payload_len = ip_total_len - ip_header_len - tcp_header_len;
        }

        info += QString(" Len=%1").arg(payload_len);

        if (p.tcp.has_timestamp) {
            info += QString(" TSval=%1 TSecr=%2")
            .arg(p.tcp.ts_val)
                .arg(p.tcp.ts_ecr);
        }

        return info;
    }
    if (p.is_udp) {
        return QString("%1 → %2 Len=%3")
            .arg(p.udp.src_port)
            .arg(p.udp.dest_port)
            .arg(p.udp.length - 8);
    }

    if (p.is_arp) {
        if (p.arp.opcode == 1) { // Request
            return QString("Who has %1? Tell %2")
                .arg(ipToString(p.arp.target_ip))
                .arg(ipToString(p.arp.sender_ip));
        } else { // Reply
            return QString("%1 is at %2")
                .arg(ipToString(p.arp.sender_ip))
                .arg(macToString(p.arp.sender_mac));
        }
    }
    if (p.is_icmp) {
        // (Sử dụng hàm static mới từ ICMPParser)
        QString info = QString::fromStdString(ICMPParser::getTypeString(p.icmp.type));

        // (Thêm 'request' hoặc 'reply' cho ping)
        if (p.icmp.type == 8) info += " (ping) request";
        if (p.icmp.type == 0) info += " (ping) reply";

        // (Thêm id và seq, LẤY TTL TỪ IPV4)
        if ((p.icmp.type == 0 || p.icmp.type == 8) && p.is_ipv4) {
            info += QString(", id=0x%1, seq=%2, ttl=%3")
            .arg(p.icmp.id, 4, 16, QChar('0'))
                .arg(p.icmp.sequence)
                .arg(p.ipv4.ttl); // Lấy TTL từ IP header
        }
        return info;
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

QString PacketTable::ipToString(uint32_t ip_host_order)
{
    // (Đã đúng)
    return QString("%1.%2.%3.%4")
        .arg((ip_host_order) & 0xFF)
        .arg((ip_host_order >> 8)  & 0xFF)
        .arg((ip_host_order >> 16) & 0xFF)
        .arg((ip_host_order >> 24) & 0xFF);
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
