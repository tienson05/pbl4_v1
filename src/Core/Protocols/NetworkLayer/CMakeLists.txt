# Đã sửa lỗi gõ nhầm "Netwrok" -> "Network"
# Thêm "STATIC"
add_library(NetworkLayerLib STATIC
    ICMPParser.cpp
    ICMPParser.hpp
    ARPParser.cpp
    ARPParser.hpp
    IPv4Parser.cpp
    IPv4Parser.hpp
    IPv6Parser.cpp
    IPv6Parser.hpp
)

# Thêm dòng này để các thư viện khác có thể include header
target_include_directories(NetworkLayerLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Thêm dòng này vì các file parser của bạn (ví dụ ARPParser)
# cần include "PacketData.hpp" từ CommonLib
target_link_libraries(NetworkLayerLib PUBLIC
    CommonLib
)
