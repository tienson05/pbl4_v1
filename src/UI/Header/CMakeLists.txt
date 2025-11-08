add_library(HeaderLib STATIC
    HeaderWidget.cpp
    HeaderWidget.hpp
    FileMenu.cpp
    FileMenu.hpp
    CaptureMenu.cpp
    CaptureMenu.hpp
    AnalyzeMenu.cpp
    AnalyzeMenu.hpp
)

# Cho phép các module khác include file header trong UI/
target_include_directories(HeaderLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(HeaderLib
    PUBLIC
        Qt6::Widgets
)
