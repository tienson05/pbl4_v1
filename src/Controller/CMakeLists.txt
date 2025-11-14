# src/Controller/CMakeLists.txt

# 1. Thêm TẤT CẢ các file .cpp và .hpp trong thư mục này
add_library(ControllerLib STATIC
    AppController.cpp
    AppController.hpp
    ControllerLib/DisplayFilterEngine.cpp
    ControllerLib/DisplayFilterEngine.hpp
)

# 2. Tìm libpcap (vì DisplayFilterEngine cần nó)
set(PCAP_ROOT ${CMAKE_SOURCE_DIR}/third_party/libpcap)
include_directories(${PCAP_ROOT}/include)
link_directories(${PCAP_ROOT}/lib)

# 3. Cho phép các module khác (#include "AppController.hpp")
target_include_directories(ControllerLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 4. Liên kết (link) với các thư viện mà ControllerLib cần
target_link_libraries(ControllerLib
    PUBLIC
        # Thư viện tùy chỉnh
        CommonLib         # Cần cho PacketData.hpp
        CaptureLib        # Cần cho CaptureEngine.hpp
        UILib             # Cần cho MainWindow.hpp

        # Thư viện Qt và Pcap
        Qt6::Core
        ${PCAP_ROOT}/lib/libpcap.so # Cần cho pcap_compile, ...
)
