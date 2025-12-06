# --- SỬA: Thay thế Qt${QT_VERSION_MAJOR} bằng Qt6 trực tiếp ---
find_package(Qt6 COMPONENTS Core Widgets Charts REQUIRED)

add_executable(App
    main.cpp
)

set(PCAP_ROOT ${CMAKE_SOURCE_DIR}/third_party/libpcap)

target_link_libraries(App
    PRIVATE
    # --- SỬA: Dùng Qt6:: thay vì biến ---
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts   # <-- Quan trọng: Link Charts vào đây

    # Các thư viện của bạn
    WidgetsLib      # (Kiểm tra lại tên: Nếu trong src/UI/Widgets/CMakeLists.txt bạn đặt là UILib hay WidgetsLib?)
    ControllerLib
    CaptureLib
    ApplicationLayerLib
    TransportLayerLib
    NetworkLayerLib
    LinkLayerLib
    CommonLib

    ${PCAP_ROOT}/lib/libpcap.so
)
