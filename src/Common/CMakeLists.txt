add_library(CommonLib INTERFACE)

# Cho phép các target khác include header
target_include_directories(CommonLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
