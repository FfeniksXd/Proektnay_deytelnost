#pragma once
#include "IPlayer.h"

// Лёгкий уровень: случайный ход среди пустых клеток
class EasyAI : public IPlayer {
public:
    std::optional<Position> chooseMove(
        const Board& board, const GameRules& rules, Cell self) override;
};