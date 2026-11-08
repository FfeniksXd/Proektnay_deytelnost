#pragma once
#include "Board.h"
#include "GameRules.h"
#include <optional>

class IPlayer {
public:
    virtual ~IPlayer() = default;
    // Возвращает ход, который выбрал игрок
    virtual std::optional<Position> chooseMove(
        const Board& board, const GameRules& rules, Cell self) = 0;
};