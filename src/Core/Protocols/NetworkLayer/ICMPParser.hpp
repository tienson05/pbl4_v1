#ifndef ICMP_PARSER_HPP
#define ICMP_PARSER_HPP

#include "../../../Common/PacketData.hpp"
#include <string>
#include <cstdint>

class ICMPParser {
public:
    /**
     * @brief Phân tích (parse) header ICMP từ dữ liệu thô.
     */
    static bool parse(ICMPHeader& icmp, const uint8_t* data, size_t len);

    /**
     * @brief Thêm thông tin ICMP vào cây chi tiết (tree view).
     */
    static void appendTreeView(std::string& tree, int depth, const ICMPHeader& icmp);

    /**
     * @brief (MỚI) Hàm helper để lấy tên Type (Loại)
     */
    static std::string getTypeString(uint8_t type);
};

#endif // ICMP_PARSER_HPP
