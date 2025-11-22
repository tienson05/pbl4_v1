// src/Controller/ControllerLib/DisplayFilterEngine.cpp

#include "DisplayFilterEngine.hpp"
#include <QStringList>
#include <QRegularExpression>
DisplayFilterEngine::DisplayFilterEngine() {}

// --- HÀM HELPER CHUYỂN ĐỔI IP (Copy logic từ PacketTable sang) ---
QString DisplayFilterEngine::ipToString(uint32_t ip) {
    return QString("%1.%2.%3.%4")
    .arg((ip) & 0xFF)
        .arg((ip >> 8) & 0xFF)
        .arg((ip >> 16) & 0xFF)
        .arg((ip >> 24) & 0xFF);
}

bool DisplayFilterEngine::match(const PacketData& packet, const QString& filterText) {
    QString filter = filterText.trimmed().toLower();
    if (filter.isEmpty()) return true;

    // 1. Xử lý OR (||) trước
    // Ví dụ: "tcp.port == 80 || udp.port == 53"
    // Nó sẽ tách thành 2 phần. Nếu MỘT trong các phần đúng -> Trả về True.
    QStringList orParts = filter.split("||");

    for (const QString& orPart : orParts) {
        // Với mỗi phần của OR, ta kiểm tra xem có AND (&&) bên trong không
        // Ví dụ: "ip.src == 1.1.1.1 && tcp.port == 80"
        QStringList andParts = orPart.split("&&");

        bool isAndBlockValid = true; // Giả sử khối AND này đúng

        for (const QString& subPart : andParts) {
            // Kiểm tra từng điều kiện đơn lẻ
            if (!matchSingleCondition(packet, subPart.trimmed())) {
                isAndBlockValid = false; // Nếu 1 cái sai -> Cả khối AND sai
                break;
            }
        }

        // Nếu cả khối AND này đúng -> Thì cả biểu thức OR lớn cũng đúng
        if (isAndBlockValid) {
            return true;
        }
    }

    // Nếu chạy hết vòng lặp mà không có khối nào đúng -> Return false
    return false;
}
bool DisplayFilterEngine::matchSingleCondition(const PacketData& packet, const QString& condition) {
    if (condition.isEmpty()) return true;

    // 1. Lọc Protocol (Không có toán tử)
    // Kiểm tra nếu chuỗi không chứa bất kỳ ký tự so sánh nào
    if (!condition.contains(QRegularExpression("[=!<>]=?"))) {
        return checkProtocol(packet, condition);
    }

    // 2. Xác định toán tử
    QString op = "";
    int opIndex = -1;

    // Thứ tự quan trọng: Kiểm tra 2 ký tự trước (==, !=, >=, <=)
    QStringList twoCharOps = {"==", "!=", ">=", "<="};
    for (const QString& o : twoCharOps) {
        opIndex = condition.indexOf(o);
        if (opIndex != -1) {
            op = o;
            break;
        }
    }

    // Nếu không tìm thấy 2 ký tự, tìm 1 ký tự (>, <)
    if (op.isEmpty()) {
        if (condition.contains(">")) { op = ">"; opIndex = condition.indexOf(">"); }
        else if (condition.contains("<")) { op = "<"; opIndex = condition.indexOf("<"); }
        else if (condition.contains("=")) { op = "=="; opIndex = condition.indexOf("="); } // Hỗ trợ gõ 1 dấu =
    }

    if (op.isEmpty() || opIndex == -1) return false; // Lỗi cú pháp

    // 3. Tách Key và Value
    QString key = condition.left(opIndex).trimmed();
    QString valueStr = condition.mid(opIndex + op.length()).trimmed();
    valueStr.remove('\"').remove('\'');

    // 4. Gọi hàm kiểm tra tương ứng
    if (key == "ip.addr" || key == "ip.src" || key == "ip.dst") {
        return checkIp(packet, valueStr, key, op);
    }
    if (key == "tcp.port" || key == "udp.port" || key == "port") {
        return checkPort(packet, valueStr.toInt(), key, op);
    }
    if (key == "frame.len" || key == "length") {
        return checkLength(packet, valueStr.toInt(), op);
    }

    return false;
}
bool DisplayFilterEngine::checkProtocol(const PacketData& packet, const QString& protocol) {
    if (protocol == "http") return packet.app.protocol == "HTTP";
    if (protocol == "dns")  return packet.app.protocol == "DNS";
    if (protocol == "mdns") return packet.app.protocol == "MDNS"; // Thêm MDNS
    if (protocol == "tls")  return packet.app.protocol == "TLS";
    if (protocol == "ssdp") return packet.app.protocol == "SSDP";
    if (protocol == "quic") return packet.app.protocol == "QUIC";

    if (protocol == "tcp")  return packet.is_tcp;
    if (protocol == "udp")  return packet.is_udp;
    if (protocol == "icmp") return packet.is_icmp;
    if (protocol == "arp")  return packet.is_arp;

    return false;
}
bool DisplayFilterEngine::compareInt(int val, int target, const QString& op) {
    if (op == "==") return val == target;
    if (op == "!=") return val != target;
    if (op == ">")  return val > target;
    if (op == "<")  return val < target;
    if (op == ">=") return val >= target;
    if (op == "<=") return val <= target;
    return false;
}

// --- HÀM QUAN TRỌNG BẠN ĐANG CẦN ---
bool DisplayFilterEngine::checkIp(const PacketData& packet, const QString& targetIp, const QString& type, const QString& op) {
    if (!packet.is_ipv4) return false;
    QString pktSrc = ipToString(packet.ipv4.src_ip);
    QString pktDst = ipToString(packet.ipv4.dest_ip);

    // IP chỉ hỗ trợ == và != (So sánh lớn bé với IP string rất phức tạp, tạm bỏ qua)
    bool matchSrc = (pktSrc == targetIp);
    bool matchDst = (pktDst == targetIp);

    if (op == "==") {
        if (type == "ip.src") return matchSrc;
        if (type == "ip.dst") return matchDst;
        if (type == "ip.addr") return matchSrc || matchDst;
    }
    else if (op == "!=") {
        if (type == "ip.src") return !matchSrc; // Src KHÔNG phải là target
        if (type == "ip.dst") return !matchDst;
        // ip.addr != X nghĩa là: Cả nguồn và đích đều KHÔNG PHẢI là X
        if (type == "ip.addr") return !matchSrc && !matchDst;
    }

    return false;
}

// --- (NÂNG CẤP) CHECK PORT VỚI TOÁN TỬ ---
bool DisplayFilterEngine::checkPort(const PacketData& packet, int targetPort, const QString& type, const QString& op) {
    uint16_t src = 0, dst = 0;
    bool hasPort = false;

    // Lấy port dựa trên giao thức
    if (packet.is_tcp && (type == "tcp.port" || type == "port")) {
        src = packet.tcp.src_port;
        dst = packet.tcp.dest_port;
        hasPort = true;
    }
    else if (packet.is_udp && (type == "udp.port" || type == "port")) {
        src = packet.udp.src_port;
        dst = packet.udp.dest_port;
        hasPort = true;
    }

    if (!hasPort) return false; // Gói tin không có port phù hợp -> Loại

    // Logic so sánh
    bool srcMatch = compareInt(src, targetPort, op);
    bool dstMatch = compareInt(dst, targetPort, op);

    // Logic đặc biệt cho !=
    if (op == "!=") {
        // port != 80 nghĩa là cả src và dst đều KHÔNG được bằng 80
        // Nếu src=80, dst=5000 -> Gói này "dính" port 80 -> Phải loại.
        // Vậy điều kiện giữ lại là: src != 80 VÀ dst != 80
        return srcMatch && dstMatch;
    }

    // Logic cho ==, >, <, >=, <=
    // port > 1024 nghĩa là src > 1024 HOẶC dst > 1024
    return srcMatch || dstMatch;
}
// --- (MỚI) LỌC THEO ĐỘ DÀI ---
bool DisplayFilterEngine::checkLength(const PacketData& packet, int targetLen, const QString& op) {
    return compareInt(packet.wire_length, targetLen, op);
}
