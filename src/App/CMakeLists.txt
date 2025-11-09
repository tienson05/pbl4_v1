# This is executable file -> use add_executable
# File App chỉ nên chứa hàm main()
add_executable(App
    main.cpp
)

# --- THAY ĐỔI QUAN TRỌNG ---
# Bạn phải liên kết (link) file thực thi App
# với TẤT CẢ các thư viện (static libs) mà nó cần.

target_link_libraries(App
    PRIVATE
    # Thư viện Qt
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets

    # --- Thư viện tùy chỉnh của bạn ---

    # Giao diện (UI)
    UILib         # (Chứa MainWindow, Pages, Widgets)

    # Bộ điều khiển (Controller)
    ControllerLib # (Chứa AppController)

    # Lõi (Core)
    CaptureLib    # (Chứa CaptureEngine, Parser)

    # Các thư viện Protocol (Parser cần chúng)
    ApplicationLayerLib
    TransportLayerLib
    NetworkLayerLib
    LinkLayerLib

    # Dữ liệu chung (Common)
    CommonLib
)
