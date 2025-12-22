add_library(HeaderLib STATIC
    HeaderWidget.cpp
    HeaderWidget.hpp
    FileMenu.cpp
    FileMenu.hpp


    AnalyzeMenu.cpp
    AnalyzeMenu.hpp
)

# Cho phép các module khác (#include "HeaderWidget.hpp")
target_include_directories(HeaderLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Liên kết với các thư viện mà HeaderLib cần
target_link_libraries(HeaderLib PUBLIC
    Qt6::Core
    Qt6::Widgets
)
