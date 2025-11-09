# src/CMakeLists.txt

# 1. Thêm Common (không phụ thuộc vào ai)
# (Chứa PacketData.hpp)
add_subdirectory(Common/)

# 2. Thêm Core (chứa Capture, Protocols - phụ thuộc vào Common)
# (Đây là nơi ApplicationLayerLib được ĐỊNH NGHĨA)
add_subdirectory(Core/)

# 3. Thêm Controller (phụ thuộc vào Core, Common)
add_subdirectory(Controller/)

# 4. Thêm UI (chứa Pages, Widgets - phụ thuộc vào Core, Controller)
add_subdirectory(UI/)

# 5. Thêm App (phụ thuộc vào TẤT CẢ ở trên)
# Lệnh này PHẢI ở CUỐI CÙNG
add_subdirectory(App/)
