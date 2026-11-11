#pragma once
#include "IPlayer.h"
#include <random>

class AIPlayer : public IPlayer {
public:
    AIPlayer() = default;
    
    // Переопределяем метод из IPlayer
    std::optional<Position> chooseMove(
        const Board& board, const GameRules& rules, Cell self) override;
};