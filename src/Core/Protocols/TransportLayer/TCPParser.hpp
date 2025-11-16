#ifndef TCP_PARSER_HPP
#define TCP_PARSER_HPP

#include "../../../Common/PacketData.hpp" // (Quan trọng)
#include <string>
#include <cstdint>

class TCPParser {
public:
    /**
     * @brief Phân tích (parse) header TCP từ dữ liệu thô.
     * @param tcp Struct TCPHeader (trong PacketData) để điền dữ liệu vào.
     * @param data Con trỏ đến đầu của header TCP.
     * @param len Kích thước còn lại của gói tin.
     * @return true nếu phân tích thành công.
     */
    static bool parse(TCPHeader& tcp, const uint8_t* data, size_t len);

    /**
     * @brief Thêm thông tin TCP vào cây chi tiết (tree view).
     * @param tree Chuỗi tree view (được truyền tham chiếu).
     * @param depth Độ sâu hiện tại của cây.
     * @param tcp Struct TCPHeader đã được điền.
     */
    static void appendTreeView(std::string& tree, int depth, const TCPHeader& tcp);

private:
    // --- (THÊM MỚI) ---
    /**
     * @brief Hàm helper (nội bộ) để phân tích các tùy chọn TCP.
     * @param tcp Struct TCPHeader (để điền TSval, TSecr).
     * @param options_data Con trỏ đến đầu của mảng options.
     * @param options_len Độ dài của mảng options (tính bằng byte).
     */
    static void parseTCPOptions(TCPHeader& tcp, const uint8_t* options_data, size_t options_len);
    // --- KẾT THÚC THÊM MỚI ---
};

#endif // TCP_PARSER_HPP
