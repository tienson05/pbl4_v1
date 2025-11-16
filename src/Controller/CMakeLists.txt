# src/Controller/CMakeLists.txt (File của Controller)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(ControllerLib STATIC
    # Các file .cpp
    AppController.cpp
    ControllerLib/DisplayFilterEngine.cpp
    StatisticsManager.cpp

    # CÁC FILE .HPP CÓ Q_OBJECT / SIGNALS
    AppController.hpp
    ControllerLib/DisplayFilterEngine.hpp
    StatisticsManager.hpp
    ControllerLib/ConversationManager.hpp ControllerLib/ConversationManager.cpp
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

# Liên kết (link) các thư viện
target_link_libraries(ControllerLib
    PUBLIC
        UILib
        CaptureLib
        CommonLib
        Qt6::Core
        Qt6::Widgets     # <-- Bắt buộc phải có
        ${PCAP_ROOT}/lib/libpcap.so
)
