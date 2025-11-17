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
#include <QTextBlock>
// --- (MỚI) HÀM HELPER ĐỂ CHUYỂN SANG HEX (GIỐNG WIRESHARK) ---
template <typename T>
static QString toHex(T val, int width = 4) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(width) << std::setfill('0') << static_cast<int>(val);
    return QString::fromStdString(ss.str());
}
// === (MỚI) CÁC THAM SỐ TỐI ƯU HÓA ===
const int CHUNK_SIZE = 500; // Xử lý 500 gói mỗi lần
const int TIMER_INTERVAL_MS = 30; // ~33 FPS

PacketTable::PacketTable(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    // Kết nối sự kiện click dòng
    connect(packetList, &QTableWidget::itemClicked, this, &PacketTable::onPacketRowSelected);
    connect(packetDetails, &QTreeWidget::itemClicked, this, &PacketTable::onDetailRowSelected);

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
    // (THÊM MỚI) Cho phép hiển thị HTML
    packetBytes->setAcceptRichText(true);
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
    m_currentSelectedPacket.clear(); // (MỚI) Xóa gói tin đang chọn

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
    if (!item) return; // (Thêm Guard)
    int row = item->row();
    QTableWidgetItem *hidden = packetList->item(row, 7); // (Đã đúng: Cột 7)
    if (!hidden) return;
m_currentSelectedPacket = hidden->data(Qt::UserRole).value<PacketData>();
    showPacketDetails(m_currentSelectedPacket);
    showHexDump(m_currentSelectedPacket,-1,0);
}
// === (MỚI) SLOT ĐỂ TÔ ĐẬM ===
void PacketTable::onDetailRowSelected(QTreeWidgetItem *item, int column)
{
    if (!item) return;

    // 1. Lấy vị trí (offset) và độ dài (length) từ item
    // (Chúng ta dùng UserRole + 1 và + 2 để lưu 2 giá trị)
    QVariant offsetData = item->data(0, Qt::UserRole + 1);
    QVariant lengthData = item->data(0, Qt::UserRole + 2);

    if (offsetData.isValid() && lengthData.isValid()) {
        int offset = offsetData.toInt();
        int length = lengthData.toInt();

        // 2. Gọi lại showHexDump với thông tin tô đậm
        // (Chúng ta dùng m_currentSelectedPacket đã lưu)
        showHexDump(m_currentSelectedPacket, offset, length);
    } else {
        // Nếu nhấp vào một mục cha (không có byte cụ thể),
        // hiển thị hex không tô đậm
        showHexDump(m_currentSelectedPacket, -1, 0);
    }
}
// === HIỂN THỊ CHI TIẾT 4 TẦNG ===
// === (HÀM ĐÃ ĐƯỢC NÂNG CẤP HOÀN TOÀN) ===
void PacketTable::showPacketDetails(const PacketData &packet)
{
    packetDetails->clear();

    // (Các offset (vị trí byte) được tính toán)
    int eth_offset = 0;
    int eth_len = 14; // (Độ dài mặc định)
    int l3_offset = eth_offset + 14; // Mặc định của Ethernet
    int l4_offset = -1;
    int l7_offset = -1;
    // --- TẦNG 1: FRAME ---
    QTreeWidgetItem *root = new QTreeWidgetItem(packetDetails);
    root->setText(0, QString("Frame %1: %2 bytes on wire (%3 bits), %4 bytes captured (%5 bits)")
                         .arg(packet.packet_id)
                         .arg(packet.wire_length)
                         .arg(packet.wire_length * 8)
                         .arg(packet.cap_length)
                         .arg(packet.cap_length * 8));
    root->setData(0, Qt::UserRole + 1, 0);
    root->setData(0, Qt::UserRole + 2, packet.cap_length);
    // Chuyển đổi timestamp
    QDateTime timestamp = QDateTime::fromSecsSinceEpoch(packet.timestamp.tv_sec);
    timestamp = timestamp.addMSecs(packet.timestamp.tv_nsec / 1000000);
    // (Chúng ta dùng toLocalTime() để hiển thị múi giờ của người dùng)
    addField(root, "Arrival Time", timestamp.toLocalTime().toString("MMM d, yyyy hh:mm:ss.zzz"));
    addField(root, "Frame Number", QString::number(packet.packet_id));
    addField(root, "Frame Length", QString("%1 bytes").arg(packet.wire_length));
    addField(root, "Capture Length", QString("%1 bytes").arg(packet.cap_length));


    // --- TẦNG 2: ETHERNET ---
    QTreeWidgetItem *eth = new QTreeWidgetItem(root);
    eth->setText(0, "Ethernet II");
    eth->setData(0, Qt::UserRole + 1, eth_offset);
    eth->setData(0, Qt::UserRole + 2, eth_len); // 14 byte
    addField(eth, "Destination", macToString(packet.eth.dest_mac), eth_offset + 0, 6);
    addField(eth, "Source", macToString(packet.eth.src_mac), eth_offset + 6, 6);
    addField(eth, "Type", QString("%1 (%2)")
                              .arg(toHex(packet.eth.ether_type))
                              .arg(getEtherTypeName(packet.eth.ether_type)), eth_offset + 12, 2);

    if (packet.has_vlan) {
        l3_offset += 4; // Bỏ qua 4 byte VLAN
        QTreeWidgetItem *vlan = new QTreeWidgetItem(root);
        vlan->setText(0, "802.1Q Virtual LAN");
        addField(vlan, "TPID", toHex(packet.vlan.tpid), eth_offset + 14, 2);
        addField(vlan, "TCI (Priority, DEI, ID)", toHex(packet.vlan.tci), eth_offset + 16, 2);
        addField(vlan, "Type", QString("%1 (%2)")
                                   .arg(toHex(packet.vlan.ether_type))
                                   .arg(getEtherTypeName(packet.vlan.ether_type)), eth_offset + 18, 2);
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
        addField(ip, "Header Length", QString("%1 bytes (%2)").arg(packet.ipv4.ihl * 4).arg(packet.ipv4.ihl));
        addField(ip, "Differentiated Services Field", toHex(packet.ipv4.tos, 2), l3_offset + 1, 1);
        addField(ip, "Total Length", QString::number(packet.ipv4.total_length), l3_offset + 2, 2);
        addField(ip, "Identification", toHex(packet.ipv4.id), l3_offset + 4, 2);

        // Phân tích Flags
        uint8_t flags_val = (packet.ipv4.flags_frag_offset >> 13) & 0x7;
        QString flags_str = QString("0x%1 (").arg(flags_val, 1, 16);
        if (flags_val & 0x2) flags_str += "Don't Fragment, ";
        if (flags_val & 0x1) flags_str += "More Fragments, ";
        if (!flags_str.contains(QChar(','))) flags_str += "No Flags"; else flags_str.chop(2); // Xóa ", "
        flags_str += ")";
        addField(ip, "Flags", flags_str);

        addField(ip, "Flags/Fragment Offset", toHex(packet.ipv4.flags_frag_offset), l3_offset + 6, 2);
            addField(ip, "Time to Live (TTL)", QString::number(packet.ipv4.ttl), l3_offset + 8, 1);
        addField(ip, "Protocol", QString("%1 (%2)").arg(getIpProtoName(packet.ipv4.protocol)).arg(packet.ipv4.protocol), l3_offset + 9, 1);
        addField(ip, "Header Checksum", toHex(packet.ipv4.header_checksum), l3_offset + 10, 2);
        addField(ip, "Source Address", ipToString(packet.ipv4.src_ip), l3_offset + 12, 4);
        addField(ip, "Destination Address", ipToString(packet.ipv4.dest_ip), l3_offset + 16, 4);
    }
    else if (packet.is_ipv6) {
        QTreeWidgetItem *ip = new QTreeWidgetItem(root);
        ip->setText(0, "Internet Protocol Version 6");
        ip->setData(0, Qt::UserRole + 1, l3_offset);
        ip->setData(0, Qt::UserRole + 2, 40);
        addField(ip, "Source", ipv6ToString(packet.ipv6.src_ip), l3_offset + 8, 16);
        addField(ip, "Destination", ipv6ToString(packet.ipv6.dest_ip), l3_offset + 24, 16);
        addField(ip, "Next Header", QString::number(packet.ipv6.next_header));
        addField(ip, "Hop Limit", QString::number(packet.ipv6.hop_limit));
    }
    else if (packet.is_arp) {
        QTreeWidgetItem *arp = new QTreeWidgetItem(root);
        arp->setText(0, "Address Resolution Protocol");
        arp->setData(0, Qt::UserRole + 1, l3_offset);
        arp->setData(0, Qt::UserRole + 2, 28); // 28 byte

        addField(arp, "Hardware type", (packet.arp.hardware_type == 1 ? "Ethernet (1)" : QString::number(packet.arp.hardware_type)), l3_offset + 0, 2);
        addField(arp, "Protocol type", QString("%1 (IPv4)").arg(toHex(packet.arp.protocol_type)), l3_offset + 2, 2);
        addField(arp, "Hardware size", QString::number(packet.arp.hardware_size), l3_offset + 4, 1);
        addField(arp, "Protocol size", QString::number(packet.arp.protocol_size), l3_offset + 5, 1);
        addField(arp, "Opcode", packet.arp.opcode == 1 ? "Request (1)" : "Reply (2)", l3_offset + 6, 2);
        addField(arp, "Sender MAC", macToString(packet.arp.sender_mac), l3_offset + 8, 6);
        addField(arp, "Sender IP", ipToString(packet.arp.sender_ip), l3_offset + 14, 4);
        addField(arp, "Target MAC", macToString(packet.arp.target_mac), l3_offset + 18, 6);
        addField(arp, "Target IP", ipToString(packet.arp.target_ip), l3_offset + 24, 4);
    }

    // --- TẦNG 4: TCP/UDP/ICMP ---
    if (packet.is_tcp) {
        int tcp_hdr_len = packet.tcp.data_offset * 4;
        l7_offset = l4_offset + tcp_hdr_len; // Tính offset Tầng 7
        QTreeWidgetItem *tcp = new QTreeWidgetItem(root);
        tcp->setText(0, "Transmission Control Protocol");
        tcp->setData(0, Qt::UserRole + 1, l4_offset);
        tcp->setData(0, Qt::UserRole + 2, tcp_hdr_len);
        addField(tcp, "Source Port", QString::number(packet.tcp.src_port), l4_offset + 0, 2);
        addField(tcp, "Destination Port", QString::number(packet.tcp.dest_port), l4_offset + 2, 2);
        addField(tcp, "Sequence Number (Seq)", QString::number(packet.tcp.seq_num), l4_offset + 4, 4);
        addField(tcp, "Acknowledgment Number (Ack)", QString::number(packet.tcp.ack_num), l4_offset + 8, 4);
        addField(tcp, "Header Length", QString("%1 bytes (%2)").arg(tcp_hdr_len).arg(packet.tcp.data_offset), l4_offset + 12, 1);
        addField(tcp, "Flags", toHex(packet.tcp.flags, 2), l4_offset + 13, 1);
        addField(tcp, "Window Size", QString::number(packet.tcp.window), l4_offset + 14, 2);
        addField(tcp, "Checksum", toHex(packet.tcp.checksum), l4_offset + 16, 2);
        addField(tcp, "Urgent Pointer", QString::number(packet.tcp.urgent_pointer), l4_offset + 18, 2);

        QString flags;
        if (packet.tcp.flags & TCPHeader::SYN) flags += "SYN, ";
        if (packet.tcp.flags & TCPHeader::ACK) flags += "ACK, ";
        if (packet.tcp.flags & TCPHeader::FIN) flags += "FIN, ";
        if (packet.tcp.flags & TCPHeader::RST) flags += "RST, ";
        if (packet.tcp.flags & TCPHeader::PSH) flags += "PSH, ";
        if (packet.tcp.flags & TCPHeader::URG) flags += "URG, ";
        if (!flags.isEmpty()) flags.chop(2); // Xóa ", "
        if (packet.tcp.options_len > 0) {
            QTreeWidgetItem *opts = new QTreeWidgetItem(tcp);
            opts->setText(0, "Options");
            opts->setData(0, Qt::UserRole + 1, l4_offset + 20);
            opts->setData(0, Qt::UserRole + 2, packet.tcp.options_len);
            addField(opts, QString("(%1 bytes)").arg(packet.tcp.options_len), "", l4_offset + 20, packet.tcp.options_len);

            if (packet.tcp.has_timestamp) {
                // (Không gán offset/length cho TSval vì chúng ta chưa tìm offset chính xác)
                QTreeWidgetItem *ts = new QTreeWidgetItem(opts);
                ts->setText(0, "Timestamps");
                addField(ts, "Timestamp value (TSval)", QString::number(packet.tcp.ts_val));
                addField(ts, "Timestamp echo reply (TSecr)", QString::number(packet.tcp.ts_ecr));
            }
    }
    else if (packet.is_udp) {
            l7_offset = l4_offset + 8; // UDP Header luôn là 8 byte
            QTreeWidgetItem *udp = new QTreeWidgetItem(root);
            udp->setText(0, "User Datagram Protocol");
            udp->setData(0, Qt::UserRole + 1, l4_offset);
            udp->setData(0, Qt::UserRole + 2, 8); // 8 byte
            addField(udp, "Source Port", QString::number(packet.udp.src_port), l4_offset + 0, 2);
            addField(udp, "Destination Port", QString::number(packet.udp.dest_port), l4_offset + 2, 2);
            addField(udp, "Length", QString::number(packet.udp.length), l4_offset + 4, 2);
            addField(udp, "Checksum", toHex(packet.udp.checksum), l4_offset + 6, 2);
    }
    else if (packet.is_icmp) {
        int icmp_len = packet.cap_length - l4_offset; // (Độ dài phần còn lại)
        l7_offset = l4_offset + 8; // (Giả sử 8 byte cho Ping)
        QTreeWidgetItem *icmp = new QTreeWidgetItem(root);
        icmp->setText(0, "Internet Control Message Protocol");
        icmp->setData(0, Qt::UserRole + 1, l4_offset);
        icmp->setData(0, Qt::UserRole + 2, icmp_len);
        QString typeStr = QString::fromStdString(ICMPParser::getTypeString(packet.icmp.type));
        addField(icmp, "Type", QString("%1 (%2)").arg(packet.icmp.type).arg(typeStr), l4_offset + 0, 1);
        addField(icmp, "Code", QString::number(packet.icmp.code), l4_offset + 1, 1);
        addField(icmp, "Checksum", toHex(packet.icmp.checksum), l4_offset + 2, 2);

        if (packet.icmp.type == 0 || packet.icmp.type == 8) {
            addField(icmp, "Identifier (id)", toHex(packet.icmp.id), l4_offset + 4, 2);
            addField(icmp, "Sequence Number (seq)", toHex(packet.icmp.sequence), l4_offset + 6, 2);
        }
    }
}

    // --- TẦNG 7: APPLICATION ---
    if (l7_offset != -1) {
        int app_len = packet.cap_length - l7_offset;
        if (app_len > 0) {
        QTreeWidgetItem *app = new QTreeWidgetItem(root);
        app->setText(0, QString::fromStdString("Application: " + packet.app.protocol));
        app->setData(0, Qt::UserRole + 1, l7_offset);
        app->setData(0, Qt::UserRole + 2, app_len);
        addField(app, QString("Data (%1 bytes)").arg(app_len), "", l7_offset, app_len);
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
        // (Hiển thị info QUIC nếu có)
        else if (!packet.app.info.empty()) {
            addField(app, "Info", QString::fromStdString(packet.app.info));
        }
    }
    }
    packetDetails->expandAll();
}
// === HEX DUMP ===
void PacketTable::showHexDump(const PacketData &packet, int highlight_offset, int highlight_len)
{
    // (Dùng 'data' từ gói tin đã lưu, không phải m_currentSelectedPacket)
    const auto& data = packet.raw_packet;
    size_t len = data.size();

    // (Dùng QFont thay vì setFont)
    QFont font("Courier", 9);
    packetBytes->setFont(font);

    // (Dùng HTML để tô đậm)
    std::stringstream oss_html;
    oss_html << "<html><head><style>"
             << "body { font-family: 'Courier'; font-size: 9pt; white-space: pre; }"
             // (Màu tô đậm giống Wireshark)
             << ".hl { background-color: #a0c0f0; color: black; }"
             << "</style></head><body>";

    for (size_t i = 0; i < len; i += 16) {
        // Offset
        oss_html << "<b>" << QString("%1").arg(i, 8, 16, QChar('0')).toUpper().toStdString() << "</b>  ";

        // Hex
        for (size_t j = 0; j < 16; ++j) {
            size_t idx = i + j;
            if (idx < len) {
                // (KIỂM TRA TÔ ĐẬM)
                bool highlight = (highlight_offset != -1) &&
                                 (idx >= highlight_offset) &&
                                 (idx < (highlight_offset + highlight_len));

                if (highlight) oss_html << "<span class=\"hl\">";
                oss_html << std::hex << std::setw(2) << std::setfill('0') << (int)data[idx] << " ";
                if (highlight) oss_html << "</span>";
            }
            else {
                oss_html << "   ";
            }
            if (j == 7) oss_html << " ";
        }

        // ASCII
        oss_html << " ";
        for (size_t j = 0; j < 16 && (i + j) < len; ++j) {
            size_t idx = i + j;
            char c = data[idx];

            // (KIỂM TRA TÔ ĐẬM)
            bool highlight = (highlight_offset != -1) &&
                             (idx >= highlight_offset) &&
                             (idx < (highlight_offset + highlight_len));

            if (highlight) oss_html << "<span class=\"hl\">";

            // (Chuyển đổi ký tự HTML đặc biệt)
            if (c >= 32 && c <= 126) {
                if (c == '<') oss_html << "&lt;";
                else if (c == '>') oss_html << "&gt;";
                else if (c == '&') oss_html << "&amp;";
                else oss_html << c;
            } else {
                oss_html << ".";
            }

            if (highlight) oss_html << "</span>";
        }
        oss_html << "<br>"; // Dùng HTML line break
    }

    oss_html << "</body></html>";
    packetBytes->setHtml(QString::fromStdString(oss_html.str()));
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

void PacketTable::addField(QTreeWidgetItem *parent, const QString &name, const QString &value, int offset, int length)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, name + ": " + value);

    // (MỚI) Lưu siêu dữ liệu (metadata) vào item
    // (Chúng ta dùng UserRole + 1 và + 2 để lưu 2 giá trị)
    item->setData(0, Qt::UserRole + 1, offset);
    item->setData(0, Qt::UserRole + 2, length);
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
