# src/Controller/CMakeLists.txt (File của Controller)

add_library(ControllerLib STATIC
    # (Đã sửa đường dẫn - các file nằm ngay cạnh file này)
    AppController.cpp
    AppController.hpp
    ControllerLib/DisplayFilterEngine.cpp
    ControllerLib/DisplayFilterEngine.hpp
)

# --- THÊM MỚI: Cần đường dẫn đến libpcap ---
set(PCAP_ROOT ${CMAKE_SOURCE_DIR}/third_party/libpcap)
include_directories(${PCAP_ROOT}/include)
link_directories(${PCAP_ROOT}/lib)
# ----------------------------------------

# Cho phép các module khác include header
target_include_directories(ControllerLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/UI
    ${CMAKE_SOURCE_DIR}/src/Core/Capture
)

target_link_libraries(ControllerLib
    PUBLIC
        UILib               # vì dùng MainWindow
        CaptureLib          # vì dùng CaptureEngine
        CommonLib           # <-- THÊM MỚI (Cần cho PacketData.hpp)
        Qt6::Core           # QObject, signal/slot

        # --- THÊM MỚI: Cần cho DisplayFilterEngine ---
        ${PCAP_ROOT}/lib/libpcap.so
)
