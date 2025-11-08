# README – Hướng dẫn chạy project Wireshark Mini (Qt + libpcap)

### Cách 1: Dùng Qt Creator
1. Mở Qt Creator → chọn “Open Project” → chọn file CMakeLists.txt trong thư mục pbl_v1.
2. Chọn kit build (ví dụ: Desktop Qt 6.9.2 GCC 64-bit).
3. Nhấn Build.

### Cách 2: Build bằng terminal
    mkdir build && cd build
    cmake ..
    make -j$(nproc)

## 4. Cấp quyền cho file chạy (bắt buộc)
Tìm file thực thi:
    find ~.../pbl_v1/build -type f -executable
Thay "..." bằng đường dẫn thực tế
Sau khi build, file thực thi nằm tại:
    ~.../pbl_v1/build/Desktop_Qt_6_9_2-Debug/src/App/App

Chạy lệnh cấp quyền:
    sudo setcap cap_net_raw,cap_net_admin=eip ~.../pbl_v1/build/Desktop_Qt_6_9_2-Debug/src/App/App

Kiểm tra lại:
    getcap ~.../pbl_v1/build/Desktop_Qt_6_9_2-Debug/src/App/App

Kết quả đúng sẽ như sau:
    /home/.../App = cap_net_admin,cap_net_raw+eip

## 5. Chạy chương trình
    cd ~/Qt/Repo/pbl_v1/build/Desktop_Qt_6_9_2-Debug/src/App/
    ./App

