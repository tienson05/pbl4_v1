#pragma once

#include <QString>
#include <QTreeWidget>
#include <QTextEdit>
#include <QColor>
#include <sstream>
#include <iomanip>
#include <ctime> // <--- THÊM ĐỂ DÙNG struct timespec
#include "../../Common/PacketData.hpp"

class PacketFormatter {
public:
    static void populateTree(QTreeWidget* tree, const PacketData& packet);
    static void displayHexDump(QTextEdit* textEdit, const PacketData& packet, int hl_offset = -1, int hl_len = 0);

    // --- SỬA DÒNG NÀY ---
    // Cũ: static QString formatTime(const struct timeval& ts);
    // Mới:
    static QString formatTime(const struct timespec& ts);

    static QString getProtocolName(const PacketData& p);
    static QString getSource(const PacketData& p);
    static QString getDest(const PacketData& p);
    static QString getInfo(const PacketData& p);
    static QColor getRowColor(const QString& proto);

private:
    static void addField(QTreeWidgetItem *parent, const QString &name, const QString &value, int offset = -1, int length = 0);
    static QString getResolvedMacLabel(const QString& rawMac);
    static QString macToString(const std::array<uint8_t, 6>& mac);
    static QString ipToString(uint32_t ip);
    static QString ipv6ToString(const std::array<uint8_t, 16>& ip);
    static QString getEtherTypeName(uint16_t type);
    static QString getIpProtoName(uint8_t proto);

    template <typename T>
    static QString toHex(T val, int width = 4) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(width) << std::setfill('0') << static_cast<int>(val);
        return QString::fromStdString(ss.str());
    }
};
