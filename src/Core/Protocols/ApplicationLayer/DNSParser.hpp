#ifndef DNS_PARSER_HPP
#define DNS_PARSER_HPP

#include "../../../Common/PacketData.hpp"
#include <string>
#include <cstdint>

class DNSParser {
public:
    /**
     * @brief Phân tích (parse) payload của Tầng 7 (Application) xem có phải DNS không.
     * @param app Struct ApplicationLayer (trong PacketData) để điền dữ liệu vào.
     * @param data Con trỏ đến đầu của payload (sau header UDP).
     * @param len Kích thước còn lại của payload.
     * @return true nếu phân tích thành công (là DNS), false nếu không.
     */
    static bool parse(ApplicationLayer& app, const uint8_t* data, size_t len);

    /**
     * @brief Thêm thông tin DNS vào cây chi tiết (tree view).
     * @param tree Chuỗi tree view (được truyền tham chiếu).
     * @param depth Độ sâu hiện tại của cây.
     * @param app Struct ApplicationLayer đã được điền.
     */
    static void appendTreeView(std::string& tree, int depth, const ApplicationLayer& app);
};

#endif // DNS_PARSER_HPP
