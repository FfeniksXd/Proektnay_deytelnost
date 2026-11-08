#pragma once
#include <string>

struct GameMode {
    std::string name;
    int boardSize;   // 3 для 3x3, 4 для 4x4 и т.д.
    int winLength;   // сколько нужно в ряд для победы
    bool misere;     // true, если проигрывает тот, кто построил линию
};