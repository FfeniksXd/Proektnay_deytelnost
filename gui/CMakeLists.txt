# Создаем исполняемый файл app из main.cpp
qt_add_executable(app main.cpp)
# Линкуем с библиотекой виджетов и нашими модулями
target_link_libraries(app PRIVATE Qt6::Widgets session core)