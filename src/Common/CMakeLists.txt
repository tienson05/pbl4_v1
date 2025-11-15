add_library(CommonLib INTERFACE)

# Cho phép các target khác include header (Dòng này đã đúng)
target_include_directories(CommonLib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# --- (THÊM ĐOẠN NÀY) ---
# Vì "CommonLib" là INTERFACE và PacketData.hpp (header)
# của nó #include <QVariant> (từ Qt6::Core),
# chúng ta phải "bảo" bất kỳ ai link với CommonLib
# rằng họ cũng PHẢI link với Qt6::Core.
target_link_libraries(CommonLib INTERFACE
    Qt6::Core
)



