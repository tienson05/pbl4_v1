
#include "DisplayFilterEngine.hpp"
#include "../../Common/PacketData.hpp" // Đảm bảo include PacketData
#include <QStringList>
#include <QRegularExpression>

DisplayFilterEngine::DisplayFilterEngine() {}

// --- HÀM HELPER CHUYỂN ĐỔI IP ---
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

    // 1. Xử lý OR (||)
    QStringList orParts = filter.split("||");

    for (const QString& orPart : orParts) {
        // 2. Xử lý AND (&&)
        QStringList andParts = orPart.split("&&");
        bool isAndBlockValid = true;

        for (const QString& subPart : andParts) {
            // Kiểm tra từng điều kiện đơn lẻ
            if (!matchSingleCondition(packet, subPart.trimmed())) {
                isAndBlockValid = false;
                break;
            }
        }

        if (isAndBlockValid) {
            return true;
        }
    }

    return false;
}

bool DisplayFilterEngine::matchSingleCondition(const PacketData& packet, const QString& condition) {
    if (condition.isEmpty()) return true;

    // 1. Lọc Protocol (Không có toán tử)
    if (!condition.contains(QRegularExpression("[=!<>]=?"))) {
        return checkProtocol(packet, condition);
    }

    // 2. Xác định toán tử
    QString op = "";
    int opIndex = -1;

    QStringList twoCharOps = {"==", "!=", ">=", "<="};
    for (const QString& o : twoCharOps) {
        opIndex = condition.indexOf(o);
        if (opIndex != -1) {
            op = o;
            break;
        }
    }

    if (op.isEmpty()) {
        if (condition.contains(">")) { op = ">"; opIndex = condition.indexOf(">"); }
        else if (condition.contains("<")) { op = "<"; opIndex = condition.indexOf("<"); }
        else if (condition.contains("=")) { op = "=="; opIndex = condition.indexOf("="); }
    }

    if (op.isEmpty() || opIndex == -1) return false;

    // 3. Tách Key và Value
    QString key = condition.left(opIndex).trimmed();
    QString valueStr = condition.mid(opIndex + op.length()).trimmed();
    valueStr.remove('\"').remove('\'');

    // 4. Gọi hàm kiểm tra tương ứng

    // --- (MỚI) HỖ TRỢ LỌC STREAM ---
    if (key == "stream") {
        // Nếu gói tin không thuộc luồng nào (stream_index = -1) -> Không bao giờ khớp
        if (packet.stream_index < 0) return false;

        // So sánh (stream_index là số nguyên, dùng compareInt cho tiện)
        return compareInt((int)packet.stream_index, valueStr.toInt(), op);
    }
    // -------------------------------

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
    if (protocol == "mdns") return packet.app.protocol == "MDNS";
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

bool DisplayFilterEngine::checkIp(const PacketData& packet, const QString& targetIp, const QString& type, const QString& op) {
    if (!packet.is_ipv4) return false;
    QString pktSrc = ipToString(packet.ipv4.src_ip);
    QString pktDst = ipToString(packet.ipv4.dest_ip);

    bool matchSrc = (pktSrc == targetIp);
    bool matchDst = (pktDst == targetIp);

    if (op == "==") {
        if (type == "ip.src") return matchSrc;
        if (type == "ip.dst") return matchDst;
        if (type == "ip.addr") return matchSrc || matchDst;
    }
    else if (op == "!=") {
        if (type == "ip.src") return !matchSrc;
        if (type == "ip.dst") return !matchDst;
        if (type == "ip.addr") return !matchSrc && !matchDst;
    }

    return false;
}

bool DisplayFilterEngine::checkPort(const PacketData& packet, int targetPort, const QString& type, const QString& op) {
    uint16_t src = 0, dst = 0;
    bool hasPort = false;

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

    if (!hasPort) return false;

    bool srcMatch = compareInt(src, targetPort, op);
    bool dstMatch = compareInt(dst, targetPort, op);

    if (op == "!=") {
        return srcMatch && dstMatch;
    }

    return srcMatch || dstMatch;
}

bool DisplayFilterEngine::checkLength(const PacketData& packet, int targetLen, const QString& op) {
    return compareInt(packet.wire_length, targetLen, op);
}
