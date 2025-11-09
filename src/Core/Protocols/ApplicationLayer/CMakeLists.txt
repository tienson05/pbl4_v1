# src/Core/Protocols/ApplicationLayer/CMakeLists.txt

# Đảm bảo nó là STATIC
add_library(ApplicationLayerLib STATIC
    ApplicationParser.cpp
    ApplicationParser.hpp
    HTTPParser.cpp
    HTTPParser.hpp
    DNSParser.hpp
    DNSParser.cpp
)

# Thêm dòng này để các thư viện khác có thể include header
target_include_directories(ApplicationLayerLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Thêm dòng này vì nó cần PacketData.hpp từ CommonLib
target_link_libraries(ApplicationLayerLib PUBLIC
    CommonLib
)
