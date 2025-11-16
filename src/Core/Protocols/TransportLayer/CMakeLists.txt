add_library(TransportLayerLib STATIC
    TCPParser.cpp
    TCPParser.hpp
    UDPParser.cpp
    UDPParser.hpp
)

target_link_libraries(TransportLayerLib
    PUBLIC
        CommonLib # <-- Bắt buộc (vì nó dùng PacketData.hpp)
        # (Không cần Qt6::Core ở đây nữa,
        #  vì CommonLib đã "truyền" yêu cầu đó cho chúng ta)
)
