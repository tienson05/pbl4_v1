#ifndef DISPLAYFILTERENGINE_HPP
#define DISPLAYFILTERENGINE_HPP

#include <QString>
#include "../../Common/PacketData.hpp"

class DisplayFilterEngine {
public:
    DisplayFilterEngine();

    // Hàm chính: Xử lý logic AND (&&) và OR (||)
    bool match(const PacketData& packet, const QString& filterText);

private:
    // Hàm kiểm tra điều kiện đơn (Code cũ của chúng ta chuyển vào đây)
    // Ví dụ: check "tcp.port == 80" hoặc "http"
    bool matchSingleCondition(const PacketData& packet, const QString& condition);

    bool checkProtocol(const PacketData& packet, const QString& protocol);
    bool checkIp(const PacketData& packet, const QString& targetIp, const QString& type, const QString& op);
    bool checkPort(const PacketData& packet, int targetPort, const QString& type, const QString& op);
    bool checkLength(const PacketData& packet, int targetLen, const QString& op);
    bool compareInt(int val1, int val2, const QString& op);
    QString ipToString(uint32_t ip);
};

#endif // DISPLAYFILTERENGINE_HPP
