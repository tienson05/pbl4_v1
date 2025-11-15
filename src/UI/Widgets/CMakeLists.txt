add_library(WidgetsLib STATIC
    PacketTable.cpp
    PacketTable.hpp
    StatisticsDialog.hpp StatisticsDialog.cpp
)

# Cho phép các module khác include file header trong UI/
target_include_directories(WidgetsLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Liên kết các thư viện con trong UI
target_link_libraries(WidgetsLib
    PUBLIC
        Qt6::Widgets
)
