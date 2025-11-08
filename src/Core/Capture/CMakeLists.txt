# This is lib file -> use add_library
add_library(CaptureLib STATIC
    CaptureEngine.cpp
    CaptureEngine.hpp
    InterfaceManager.cpp
    InterfaceManager.hpp
    Parser.cpp
    Parser.hpp
)

# Đường dẫn tới libpcap
set(PCAP_ROOT ${CMAKE_SOURCE_DIR}/third_party/libpcap)

# Include và lib path
include_directories(${PCAP_ROOT}/include)
link_directories(${PCAP_ROOT}/lib)

# Link thư viện động
target_link_libraries(CaptureLib
    PUBLIC
        Qt6::Core
        ${PCAP_ROOT}/lib/libpcap.so
)
