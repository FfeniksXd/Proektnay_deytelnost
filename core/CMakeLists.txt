add_library(core STATIC)
target_sources(core PRIVATE 
    Board.cpp 
    GameRules.cpp
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})