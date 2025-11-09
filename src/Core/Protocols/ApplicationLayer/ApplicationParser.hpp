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
    static bool parse(ApplicationLayer& app, const uint8_t* data, size_t len,
                      uint16_t src_port, uint16_t dest_port);

    static void appendTreeView(std::string& tree, int depth, const ApplicationLayer& app);
};

#endif // APPLICATION_PARSER_HPP
