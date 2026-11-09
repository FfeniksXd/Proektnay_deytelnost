cmake_minimum_required(VERSION 3.21)
project(TicTacToe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_PREFIX_PATH "C:/Qt/6.12.0/mingw_64")

# Подключение Qt (пока только Widgets и Test)
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
qt_standard_project_setup()

# Подключение Google Test через скачивание
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Добавляем наши модули
add_subdirectory(core)
add_subdirectory(session)
add_subdirectory(gui)
add_subdirectory(tests)