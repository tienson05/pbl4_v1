add_executable(App
    main.cpp
)

set(PCAP_ROOT ${CMAKE_SOURCE_DIR}/third_party/libpcap)

target_link_libraries(App
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets

    # Tất cả các thư viện của bạn
    UILib
    ControllerLib       # <-- Bắt buộc
    CaptureLib
    ApplicationLayerLib
    TransportLayerLib
    NetworkLayerLib
    LinkLayerLib
    CommonLib

    ${PCAP_ROOT}/lib/libpcap.so # <-- Bắt buộc
)
