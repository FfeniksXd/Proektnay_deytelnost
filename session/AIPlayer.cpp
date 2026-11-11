#include "AIPlayer.h"
#include <vector>

std::optional<Position> AIPlayer::chooseMove(const Board& board, const GameRules& rules, Cell self) {
    std::vector<Position> emptyCells;

    // Проходим по всей доске и собираем все пустые клетки
    for (int r = 0; r < board.size(); ++r) {
        for (int c = 0; c < board.size(); ++c) {
            if (board.isEmpty(r, c)) {
                emptyCells.push_back({r, c});
            }
        }
    }

    // Если ходов нет, возвращаем пустоту (ничья)
    if (emptyCells.empty()) {
        return std::nullopt;
    }

    // Выбираем случайную пустую клетку (ИИ)
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0, emptyCells.size() - 1);
    
    return emptyCells[dis(gen)];
}