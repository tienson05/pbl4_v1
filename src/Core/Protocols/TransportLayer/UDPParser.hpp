#ifndef UDP_PARSER_HPP
#define UDP_PARSER_HPP

#include "../../../Common/PacketData.hpp" // (Quan trọng)
#include <string>
#include <cstdint>

class UDPParser {
public:
    /**
     * @brief Phân tích (parse) header UDP từ dữ liệu thô.
     * @param udp Struct UDPHeader (trong PacketData) để điền dữ liệu vào.
     * @param data Con trỏ đến đầu của header UDP.
     * @param len Kích thước còn lại của gói tin.
     * @return true nếu phân tích thành công.
     */
    static bool parse(UDPHeader& udp, const uint8_t* data, size_t len);

    /**
     * @brief Thêm thông tin UDP vào cây chi tiết (tree view).
     * @param tree Chuỗi tree view (được truyền tham chiếu).
     * @param depth Độ sâu hiện tại của cây.
     * @param udp Struct UDPHeader đã được điền.
     */
    static void appendTreeView(std::string& tree, int depth, const UDPHeader& udp);
};

#endif // UDP_PARSER_HPP
