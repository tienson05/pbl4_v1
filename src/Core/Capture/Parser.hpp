#ifndef PARSER_HPP
#define PARSER_HPP

#include "../../Common/PacketData.hpp"
#include <cstddef>

class Parser {
public:
    Parser() = default;
    ~Parser() = default;

    // Parse gói tin và điền vào PacketData
    bool parse(PacketData* pkt, const uint8_t* data, size_t len);

private:
    // Helper để tránh lặp code
    void appendTree(PacketData* pkt, const std::string& line);
};
#endif
