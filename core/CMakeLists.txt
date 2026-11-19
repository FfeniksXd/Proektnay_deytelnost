add_library(core STATIC
    Board.cpp
    GameRules.cpp 
)

# Позволяем другим модулям видеть заголовочные файлы из этой папки
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})