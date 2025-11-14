# src/CMakeLists.txt

# 1. Thêm Common (không phụ thuộc vào ai)
add_subdirectory(Common/)

# 2. Thêm Core (phụ thuộc vào Common)
add_subdirectory(Core/)

# 3. Thêm Controller (phụ thuộc vào Core, Common)
add_subdirectory(Controller/)

# 4. Thêm UI (phụ thuộc vào Controller)
add_subdirectory(UI/)

# 5. Thêm App (phụ thuộc vào TẤT CẢ ở trên)
# Lệnh này PHẢI ở CUỐI CÙNG
add_subdirectory(App/)
