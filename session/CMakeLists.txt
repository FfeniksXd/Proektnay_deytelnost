add_library(session STATIC)
# Сессия зависит от модуля core
target_link_libraries(session PUBLIC core)
target_include_directories(session PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})