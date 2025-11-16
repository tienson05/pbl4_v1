#ifndef ARP_PARSER_HPP
#define ARP_PARSER_HPP

// Include PacketData vì chúng ta cần định nghĩa ARPHeader
#include "../../../Common/PacketData.hpp"
#include <string>
#include <cstdint>

class ARPParser {
public:
    /**
     * @brief Phân tích (parse) header ARP từ dữ liệu thô.
     * @param arp Struct ARPHeader (trong PacketData) để điền dữ liệu vào.
     * @param data Con trỏ đến đầu của header ARP.
     * @param len Kích thước còn lại của gói tin.
     * @return true nếu phân tích thành công.
     */
    static bool parse(ARPHeader& arp, const uint8_t* data, size_t len);

    /**
     * @brief Thêm thông tin ARP vào cây chi tiết (tree view).
     * @param tree Chuỗi tree view (được truyền tham chiếu).
     * @param depth Độ sâu hiện tại của cây.
     * @param arp Struct ARPHeader đã được điền.
     */
    static void appendTreeView(std::string& tree, int depth, const ARPHeader& arp);
};

#endif // ARP_PARSER_HPP
