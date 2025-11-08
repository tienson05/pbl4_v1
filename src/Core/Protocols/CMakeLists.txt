# # This is lib file -> use add_library
# add_library(ProtocolsLib STATIC
#     EthernetParser.hpp
#     EthernetParser.cpp
#     IPParser.cpp
#     IPParser.hpp
#     TCPParser.cpp
#     TCPParser.hpp
#     UDPParser.cpp
#     UDPParser.hpp
#     HTTPParser.cpp
#     HTTPParser.hpp
#     VLANParser.cpp
#     VLANParser.hpp
#     IPv4Parser.cpp
#     IPv4Parser.hpp
#     IPv6Parser.cpp
#     IPv6Parser.hpp
#     ICMPParser.hpp
#     ICMPParser.cpp
#     ARPParser.cpp
#     ARPParser.hpp
#     # IParser.hpp
# )

# # Đường dẫn tới thư viện libpcap trong thư mục third_party
# set(PCAP_PATH "${CMAKE_SOURCE_DIR}/third_party/libpcap")

# # Thêm include và link directory (nếu có file libpcap.so ở đó)
# include_directories(${PCAP_PATH}/include)
# link_directories(${PCAP_PATH}/lib)

# # Cho phép module khác (UI, App...) include CoreLib/
# target_include_directories(ProtocolsLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# # Liên kết libpcap
# target_link_libraries(ProtocolsLib PRIVATE
#     ${PCAP_PATH}/lib/libpcap.so
#     Qt${QT_VERSION_MAJOR}::Core
# )

add_subdirectory(ApplicationLayer/)
add_subdirectory(LinkLayer/)
add_subdirectory(NetworkLayer/)
add_subdirectory(TransportLayer/)
