#pragma once
#include "../../Common/PacketData.hpp"
#include <cstdint>

class IParser {
public:
    virtual ~IParser() = default;

    // Trả về true nếu đã parse được và có thể chuyển tiếp payload cho tầng sau
    virtual bool parse(const uint8_t* data, int len, PacketData& packetOut) = 0;
};
