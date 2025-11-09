#ifndef IPV6_PARSER_HPP
#define IPV6_PARSER_HPP

#include "../../../Common/PacketData.hpp"
#include <string>
#include <cstdint>

class IPv6Parser {
public:
    /**
     * @brief (ĐÃ THAY ĐỔI) Phân tích header IPv6 VÀ tất cả các extension header.
     * @param ipv6 Struct IPv6Header để điền dữ liệu vào.
     * @param data (Tham chiếu) Con trỏ đến đầu header IPv6. Sẽ bị thay đổi
     * để trỏ đến đầu header Tầng 4 (ví dụ: TCP/UDP).
     * @param len (Tham chiếu) Kích thước còn lại. Sẽ bị giảm đi
     * theo kích thước của header IPv6 + extension headers.
     * @return true nếu phân tích thành công.
     */
    static bool parse(IPv6Header& ipv6, const uint8_t*& data, size_t& len); // <-- ĐÃ THAY ĐỔI

    /**
     * @brief Thêm thông tin IPv6 vào cây chi tiết (tree view).
     */
    static void appendTreeView(std::string& tree, int depth, const IPv6Header& ipv6);
};

#endif // IPV6_PARSER_HPP
