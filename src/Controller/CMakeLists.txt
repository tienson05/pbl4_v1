add_library(ControllerLib STATIC
    AppController.cpp
    AppController.hpp
    ControllerLib/DisplayFilterEngine.cpp
    ControllerLib/DisplayFilterEngine.hpp
)

# Cho phép các module khác include header trong UI/
target_include_directories(ControllerLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/UI
    ${CMAKE_SOURCE_DIR}/src/Core/Capture
)

target_link_libraries(ControllerLib
    PUBLIC
        UILib           # vì dùng MainWindow
        CaptureLib         # vì dùng CaptureEngine
        Qt6::Core       # QObject, signal/slot
)
