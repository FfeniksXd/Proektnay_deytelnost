#pragma once
#include "Board.h"
#include "GameMode.h"
#include <optional>

class GameRules {
public:
    explicit GameRules(GameMode mode);
    
    bool isValidMove(const Board& board, Position p) const;
    std::optional<Cell> winner(const Board& board) const;
    bool isDraw(const Board& board) const;
    bool isGameOver(const Board& board) const;
    
    const GameMode& mode() const;
private:
    GameMode mode_;
};