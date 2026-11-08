add_library(core STATIC)
# Позволяем другим модулям видеть заголовочные файлы из этой папки
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})