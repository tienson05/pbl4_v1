#ifndef ICMP_PARSER_HPP
#define ICMP_PARSER_HPP

#include "../../../Common/PacketData.hpp" // (Quan trọng)
#include <string>
#include <cstdint>

class ICMPParser {
public:
    /**
     * @brief Phân tích (parse) header ICMP từ dữ liệu thô.
     * @param icmp Struct ICMPHeader (trong PacketData) để điền dữ liệu vào.
     * @param data Con trỏ đến đầu của header ICMP.
     * @param len Kích thước còn lại của gói tin.
     * @return true nếu phân tích thành công.
     */
    static bool parse(ICMPHeader& icmp, const uint8_t* data, size_t len);

    /**
     * @brief Thêm thông tin ICMP vào cây chi tiết (tree view).
     * @param tree Chuỗi tree view (được truyền tham chiếu).
     * @param depth Độ sâu hiện tại của cây.
     * @param icmp Struct ICMPHeader đã được điền.
     */
    static void appendTreeView(std::string& tree, int depth, const ICMPHeader& icmp);
};

#endif // ICMP_PARSER_HPP
