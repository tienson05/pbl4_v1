# Thêm các module con
add_subdirectory(Pages)
add_subdirectory(Header)
add_subdirectory(Widgets)

# --- Bật tự động xử lý Qt ---
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# --- Tạo thư viện UI chính ---
add_library(UILib STATIC
    MainWindow.cpp
    MainWindow.hpp
)

# Cho phép các module khác include header trong UI/
target_include_directories(UILib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Liên kết các thư viện con trong UI
target_link_libraries(UILib
    PUBLIC
        PagesLib        # từ add_library trong src/UI/Pages/CMakeLists.txt
        HeaderLib       # từ src/UI/Header/
        WidgetsLib      # từ src/UI/Widgets/
        CaptureLib
        Qt6::Widgets
)
