# src/App/CMakeLists.txt

add_executable(App
    main.cpp
)

# Cần tìm pcap VÌ App liên kết tĩnh (static link) với ControllerLib và CaptureLib
set(PCAP_ROOT ${CMAKE_SOURCE_DIR}/third_party/libpcap)

target_link_libraries(App
    PRIVATE
    # Thư viện Qt
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets

    # --- TẤT CẢ Thư viện tùy chỉnh của bạn ---
    UILib
    ControllerLib
    CaptureLib
    ApplicationLayerLib
    TransportLayerLib
    NetworkLayerLib
    LinkLayerLib
    CommonLib

    # --- THÊM MỚI: Liên kết pcap ở bước cuối cùng ---
    ${PCAP_ROOT}/lib/libpcap.so
)
