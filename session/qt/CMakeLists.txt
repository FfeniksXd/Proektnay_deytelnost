# Адаптеры Qt: сеть (QTcpSocket / QTcpServer) и таймеры (QTimer) для логики из session.
# Отдельная библиотека, чтобы сама session не зависела от Qt.
add_library(session_qt STATIC)

target_sources(session_qt PRIVATE
    QtTcpConnection.cpp
    QtScheduler.cpp
    QtHostServer.cpp
)

target_link_libraries(session_qt PUBLIC session Qt6::Network)
target_include_directories(session_qt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})