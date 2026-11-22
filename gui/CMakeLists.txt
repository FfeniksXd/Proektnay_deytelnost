qt_add_executable(app
    main.cpp
    gamewindow.cpp
    gamewindow.h
    gameboard.cpp
    gameboard.h
)

target_link_libraries(app PRIVATE Qt6::Widgets session core)

if(MSVC)
    target_compile_options(app PRIVATE /utf-8)
endif()
