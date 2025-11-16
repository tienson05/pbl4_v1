#ifndef APPLICATION_PARSER_HPP
#define APPLICATION_PARSER_HPP

#include "../../../Common/PacketData.hpp"
#include <string>
#include <cstdint>


/**
 * @brief Lớp "Tổng đài" (Dispatcher) cho Tầng 7.
 * Nó sẽ quyết định gọi parser con nào (HTTP, DNS, ...)
 * dựa trên số cổng (port).
 */
class ApplicationParser {
public:
    /**
     * @brief (ĐÃ SỬA) Thêm 'is_tcp' để phân biệt TCP và UDP
     */
    static bool parse(ApplicationLayer& app, const uint8_t* data, size_t len,
                      uint16_t src_port, uint16_t dest_port, bool is_tcp);

    // Hàm này dùng để hiển thị trong Packet Details
    static void appendTreeView(std::string& tree, int depth, const ApplicationLayer& app);
};

#endif // APPLICATION_PARSER_HPP
