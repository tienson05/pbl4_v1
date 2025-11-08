add_library(PagesLib STATIC
    WelcomePage.cpp
    WelcomePage.hpp
    CapturePage.cpp
    CapturePage.hpp
)

# Cho phép các module khác include file header trong UI/
target_include_directories(PagesLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Liên kết các thư viện con trong UI
target_link_libraries(PagesLib
    PUBLIC
        Qt6::Widgets
        WidgetsLib
)
