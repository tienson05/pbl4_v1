cmake_minimum_required(VERSION 3.19)
project(pbl_v1 LANGUAGES CXX)

# Yêu cầu Qt 6.4 (vì máy bạn là 6.4.2)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets)

# Tự động bật MOC, UIC, RCC
qt_standard_project_setup()

# Thêm các thư mục con
add_subdirectory(src/)
add_subdirectory(third_party/)
