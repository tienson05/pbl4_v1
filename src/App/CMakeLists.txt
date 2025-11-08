# This is executable file -> use add_execuable
add_executable(App
    main.cpp
    ../Controller/AppController.cpp
)

target_link_libraries(App
    PRIVATE Qt::Core
    PRIVATE Qt::Widgets
)

target_link_libraries(App PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    UILib
)
