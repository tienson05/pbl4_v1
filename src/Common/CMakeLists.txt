# 1. Liệt kê các file nguồn (Quan trọng: Phải có MacResolver.cpp)
set(COMMON_SOURCES
    PacketData.hpp
    MacResolver.cpp
    MacResolver.hpp
)

# 2. Đổi từ INTERFACE sang STATIC (Để biên dịch file .cpp)
add_library(CommonLib STATIC ${COMMON_SOURCES})

# 3. Thiết lập đường dẫn include
# Dùng PUBLIC để chính CommonLib dùng được và các nơi khác (App, Widgets) cũng thấy được header
target_include_directories(CommonLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 4. Link thư viện Qt
# Dùng PUBLIC để CommonLib dùng được Qt và ai link với CommonLib cũng tự động link Qt
target_link_libraries(CommonLib PUBLIC
    Qt6::Core
)
