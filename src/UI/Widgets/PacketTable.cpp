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
#include <QColor>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <QRegularExpression>
#include <QTimer> // <-- BẮT BUỘC PHẢI CÓ

// === CÁC THAM SỐ TỐI ƯU HÓA ===
/**
 * @brief Số lượng gói tin tối đa xử lý mỗi lần hẹn giờ.
 * Tăng lên nếu máy bạn mạnh, giảm xuống nếu vẫn thấy lag.
 */
const int CHUNK_SIZE = 500;

/**
 * @brief Thời gian (ms) giữa mỗi lần xử lý chunk.
 * 30ms là tốt (tương đương ~33 FPS).
 */
const int TIMER_INTERVAL_MS = 30;


PacketTable::PacketTable(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connect(packetList, &QTableWidget::itemClicked, this, &PacketTable::onPacketRowSelected);

    // --- THIẾT LẬP BỘ ĐỆM CHỐNG LAG ---
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(TIMER_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &PacketTable::processPacketChunk);
    m_updateTimer->start();
    // ------------------------------------

    // (Thêm 1 connect để theo dõi thanh cuộn)
    connect(packetList->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value){
        QScrollBar* vbar = packetList->verticalScrollBar();
        // Cập nhật m_isUserAtBottom CHỈ KHI thanh cuộn ở max
        // (Chúng ta +5 để cho phép một khoảng "lỗi" nhỏ)
        m_isUserAtBottom = (vbar == nullptr) || (value >= vbar->maximum() - 5);
    });
}

void PacketTable::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    packetList = new QTableWidget(this);
    packetList->setColumnCount(7);
    QStringList headers = {"No.", "Time", "Source", "Destination", "Protocol", "Info"};
    packetList->setHorizontalHeaderLabels(headers);

    // --- CÀI ĐẶT CỘT (GIỐNG WIRESHARK) ---
    packetList->horizontalHeader()->setStretchLastSection(true); // Cột "Info" co giãn
    packetList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // No.
    packetList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive); // Time
    packetList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive); // Source
    packetList->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive); // Destination
    packetList->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Protocol

    packetList->setSelectionBehavior(QAbstractItemView::SelectRows);
    packetList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    packetList->hideColumn(6);
    packetList->verticalHeader()->setVisible(false);

    // (Phần còn lại của setupUI giữ nguyên)
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

// (Hàm packetMatchesFilter giữ nguyên)
bool PacketTable::packetMatchesFilter(const PacketData &packet, const QString &filterText)
{
    if (filterText.isEmpty()) {
        return true; // Nếu filter rỗng, luôn hiển thị
    }

    // Tách filter thành các từ khóa
    QStringList keywords = filterText.toLower().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    // Tạo "siêu chuỗi"
    QString packetMegaString = (
                                   getSourceAddress(packet) + " " +
                                   getDestinationAddress(packet) + " " +
                                   getProtocolName(packet) + " " +
                                   getInfo(packet)
                                   ).toLower();

    // Kiểm tra TẤT CẢ từ khóa
    for (const QString &keyword : keywords) {
        if (!packetMegaString.contains(keyword)) {
            return false;
        }
    }
    return true;
}

// --- HÀM XÓA BẢNG ---
void PacketTable::clearData()
{
    packetList->setRowCount(0);
    packetDetails->clear();
    packetBytes->clear();
    // (MỚI) Xóa luôn buffer khi clear
    m_packetBuffer.clear();
}

// === THÊM GÓI TIN (SỐ ÍT - Dùng khi bắt live) ===
void PacketTable::onPacketReceived(const PacketData &packet)
{
    // (MỚI) Chỉ thêm vào hàng đợi. QTimer sẽ xử lý.
    m_packetBuffer.append(packet);
}

/**
 * @brief (SỬA) Thêm một LÔ gói tin (để chống lag)
 */
void PacketTable::onPacketsReceived(const QList<PacketData> &packets)
{
    // (MỚI) Chỉ thêm vào hàng đợi. QTimer sẽ xử lý.
    m_packetBuffer.append(packets);
}

/**
 * @brief (MỚI) Slot được gọi bởi QTimer, xử lý một lô nhỏ
 */
void PacketTable::processPacketChunk()
{
    // Nếu không có gì trong buffer, không làm gì cả
    if (m_packetBuffer.isEmpty()) {
        return;
    }

    // --- TỐI ƯU HÓA (OPTIMIZATION) ---
    packetList->setUpdatesEnabled(false);
    packetList->setSortingEnabled(false);
    // ----------------------------------

    // Tính toán số lượng sẽ xử lý lần này
int packetsToProcess = std::min(CHUNK_SIZE, (int)m_packetBuffer.size());
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

    // Tự động cuộn nếu đang ở cuối
    if (m_isUserAtBottom) {
        packetList->scrollToBottom();
    }
}

/**
 * @brief (MỚI) Hàm helper để thêm 1 hàng vào bảng
 */
void PacketTable::insertPacketRow(const PacketData &packet)
{
    // Đây chính là code bên trong vòng for CŨ của bạn

    int row = packetList->rowCount();
    packetList->insertRow(row);

    QTableWidgetItem* noItem = new QTableWidgetItem(QString::number(packet.packet_id));
    char timeStr[64];
    struct tm *tm_info = localtime(&packet.timestamp.tv_sec);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);
    QString time = QString("%1.%2").arg(timeStr).arg(packet.timestamp.tv_nsec / 1000000, 3, 10, QChar('0'));
    QString proto = getProtocolName(packet);

    QList<QTableWidgetItem*> items = {
        noItem, new QTableWidgetItem(time),
        new QTableWidgetItem(getSourceAddress(packet)),
        new QTableWidgetItem(getDestinationAddress(packet)),
        new QTableWidgetItem(proto),
        new QTableWidgetItem(getInfo(packet))
    };

    QColor color;
    if (proto == "TCP" || proto == "HTTP") color.setRgb(230, 245, 225);
    else if (proto == "TLS") color.setRgb(230, 225, 245);
    else if (proto == "UDP" || proto == "DNS") color.setRgb(225, 240, 255);
    else if (proto == "ICMP") color.setRgb(255, 245, 220);
    else if (proto == "ARP") color.setRgb(250, 240, 250);
    else color = Qt::white;

    for(int i = 0; i < items.size(); ++i) {
        items[i]->setBackground(color);
        packetList->setItem(row, i, items[i]);
    }

    QTableWidgetItem* hidden = new QTableWidgetItem();
    hidden->setData(Qt::UserRole, QVariant::fromValue(packet));
    hidden->setFlags(Qt::NoItemFlags);
    packetList->setItem(row, 6, hidden);
}


// === KHI CLICK DÒNG ===
void PacketTable::onPacketRowSelected(QTableWidgetItem *item)
{
    // (Giữ nguyên)
    int row = item->row();
    QTableWidgetItem *hidden = packetList->item(row, 6);
    if (!hidden) return;
    PacketData packet = hidden->data(Qt::UserRole).value<PacketData>();
    showPacketDetails(packet);
    showHexDump(packet);
}

void PacketTable::showPacketDetails(const PacketData &packet)
{
    // (HÀM NÀY GIỮ NGUYÊN - BẠN ĐÃ TỰ LÀM RẤT TỐT)
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
        addField(ip, "Source", ipToString(packet.ipv4.src_ip)); // Sẽ gọi hàm ipToString đã sửa
        addField(ip, "Destination", ipToString(packet.ipv4.dest_ip)); // Sẽ gọi hàm ipToString đã sửa
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
        addField(arp, "Sender IP", ipToString(packet.arp.sender_ip)); // Sẽ gọi hàm ipToString đã sửa
        addField(arp, "Target MAC", macToString(packet.arp.target_mac));
        addField(arp, "Target IP", ipToString(packet.arp.target_ip)); // Sẽ gọi hàm ipToString đã sửa
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
    // (Giữ nguyên)
    std::ostringstream oss;
    const auto& data = packet.raw_packet;
    size_t len = data.size();
    for (size_t i = 0; i < len; i += 16) {
        oss << QString("%1").arg(i, 8, 16, QChar('0')).toUpper().toStdString() << "  ";
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < len) oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i + j] << " ";
            else oss << "   ";
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
    // (Giữ nguyên)
    if (p.is_arp) return ipToString(p.arp.sender_ip);
    if (p.is_ipv4) return ipToString(p.ipv4.src_ip);
    if (p.is_ipv6) return ipv6ToString(p.ipv6.src_ip);
    return macToString(p.eth.src_mac);
}

QString PacketTable::getDestinationAddress(const PacketData &p)
{
    // (Giữ nguyên)
    if (p.is_arp) return ipToString(p.arp.target_ip);
    if (p.is_ipv4) return ipToString(p.ipv4.dest_ip);
    if (p.is_ipv6) return ipv6ToString(p.ipv6.dest_ip);
    return macToString(p.eth.dest_mac);
}

QString PacketTable::getProtocolName(const PacketData &p)
{
    // (Giữ nguyên)
    if (!p.app.protocol.empty()) { return QString::fromStdString(p.app.protocol); }
    if (p.is_tcp) return "TCP";
    if (p.is_udp) return "UDP";
    if (p.is_icmp) return "ICMP";
    if (p.is_arp) return "ARP";
    return QString("0x%1").arg(p.eth.ether_type, 4, 16, QChar('0')).toUpper();
}

QString PacketTable::getInfo(const PacketData &p)
{
    // (Giữ nguyên - Code của bạn đã đầy đủ)
    if (!p.app.info.empty()) { return QString::fromStdString(p.app.info); }
    // ( ... )
    if (p.is_tcp) {
        QString info = QString("%1 → %2 ").arg(p.tcp.src_port).arg(p.tcp.dest_port);
        QString f;
        if (p.tcp.flags & TCPHeader::SYN) f += "SYN, ";
        if (p.tcp.flags & TCPHeader::ACK) f += "ACK, ";
        if (p.tcp.flags & TCPHeader::FIN) f += "FIN, ";
        if (p.tcp.flags & TCPHeader::RST) f += "RST, ";
        if (p.tcp.flags & TCPHeader::PSH) f += "PSH, ";
        if (!f.isEmpty()) { f.chop(2); info += QString("[%1] ").arg(f); }
        info += QString("Seq=%1 Ack=%2 Win=%3").arg(p.tcp.seq_num).arg(p.tcp.ack_num).arg(p.tcp.window);
        return info;
    }
    // ( ... )
    return QString("Len=%1").arg(p.cap_length);
}

void PacketTable::addField(QTreeWidgetItem *parent, const QString &name, const QString &value)
{
    // (Giữ nguyên)
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, name + ": " + value);
}

QString PacketTable::macToString(const std::array<uint8_t, 6>& mac)
{
    // (Giữ nguyên)
    return QString("%1:%2:%3:%4:%5:%6")
        .arg(mac[0], 2, 16, QChar('0'))
        .arg(mac[1], 2, 16, QChar('0'))
        .arg(mac[2], 2, 16, QChar('0'))
        .arg(mac[3], 2, 16, QChar('0'))
        .arg(mac[4], 2, 16, QChar('0'))
        .arg(mac[5], 2, 16, QChar('0')).toUpper();
}

/**
 * @brief (ĐÃ SỬA LỖI) Chuyển đổi IP (host order) sang chuỗi.
 * Vì parser của bạn dùng ntohl(), IP được lưu ở dạng Host Order (Little Endian).
 */
QString PacketTable::ipToString(uint32_t ip_host_order)
{
    // SỬA LẠI THỨ TỰ CHO ĐÚNG (Host Order / Little Endian)
    return QString("%1.%2.%3.%4")
        .arg((ip_host_order) & 0xFF)         // Byte 1
        .arg((ip_host_order >> 8) & 0xFF)    // Byte 2
        .arg((ip_host_order >> 16) & 0xFF)   // Byte 3
        .arg((ip_host_order >> 24) & 0xFF);  // Byte 4
}


QString PacketTable::ipv6ToString(const std::array<uint8_t, 16>& ip)
{
    // (Giữ nguyên)
    QStringList parts;
    for (int i = 0; i < 16; i += 2) {
        uint16_t part = (ip[i] << 8) | ip[i + 1];
        parts << QString("%1").arg(part, 0, 16);
    }
    return parts.join(":");
}

QString PacketTable::getEtherTypeName(uint16_t type)
{
    // (Gi giữ nguyên)
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
    // (Giữ nguyên)
    switch (proto) {
    case 1: return "ICMP";
    case 6: return "TCP";
    case 17: return "UDP";
    default: return "Unknown";
    }
}
