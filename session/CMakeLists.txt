add_library(session STATIC
    AIFactory.h)

target_sources(session PRIVATE
    GameSession.cpp
    AIUtils.cpp
    EasyAI.cpp
    MediumAI.cpp
    HardAI.cpp
    AIFactory.cpp
)

# Сессия зависит от модуля core
target_link_libraries(session PUBLIC core)
target_include_directories(session PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})