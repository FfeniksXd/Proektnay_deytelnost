#include "GameRules.h"

GameRules::GameRules(GameMode mode) : mode_(mode) {}

const GameMode& GameRules::mode() const {
    return mode_;
}

bool GameRules::isValidMove(const Board& board, Position p) const {
    if (p.row < 0 || p.row >= board.size() || p.col < 0 || p.col >= board.size()) return false;
    return board.isEmpty(p.row, p.col);
}

// Временно возвращаем пустоту (потом Человек 1 напишет проверку линий)
std::optional<Cell> GameRules::winner(const Board& board) const {
    return std::nullopt;
}

bool GameRules::isDraw(const Board& board) const {
    return false;
}

bool GameRules::isGameOver(const Board& board) const {
    return winner(board).has_value() || isDraw(board);
}