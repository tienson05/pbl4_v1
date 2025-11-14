# src/UI/Pages/CMakeLists.txt

add_library(PagesLib STATIC
    # Thêm TẤT CẢ các file .cpp VÀ .hpp
    WelcomePage.cpp
    WelcomePage.hpp     # <-- BẮT BUỘC CÓ (để MOC chạy)
    CapturePage.cpp
    CapturePage.hpp     # <-- BẮT BUỘC CÓ (để MOC chạy)
)

# Cho phép các module khác (#include "CapturePage.hpp")
target_include_directories(PagesLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Liên kết với các thư viện mà PagesLib cần
target_link_libraries(PagesLib PUBLIC
    CommonLib
    WidgetsLib # Vì CapturePage dùng PacketTable
    Qt6::Core
    Qt6::Widgets
)
