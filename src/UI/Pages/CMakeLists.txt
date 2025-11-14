add_library(PagesLib STATIC
    WelcomePage.cpp
    WelcomePage.hpp
    CapturePage.cpp
    CapturePage.hpp
)

target_include_directories(PagesLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PagesLib PUBLIC
    CommonLib
    WidgetsLib # Vì CapturePage dùng PacketTable
    Qt6::Core
    Qt6::Widgets
)
