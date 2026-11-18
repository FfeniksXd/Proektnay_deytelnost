#include "AIUtils.h"
#include <cstddef>
#include <random>

std::vector<Position> emptyCells(const Board& board) {
    std::vector<Position> cells;
    for (int r = 0; r < board.size(); ++r) {
        for (int c = 0; c < board.size(); ++c) {
            if (board.isEmpty(r, c)) {
                cells.push_back({r, c});
            }
        }
    }
    return cells;
}

std::optional<Position> randomEmptyCell(const Board& board) {
    std::vector<Position> cells = emptyCells(board);
    if (cells.empty()) {
        return std::nullopt; // ходов нет
    }

    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<std::size_t> dis(0, cells.size() - 1);
    return cells[dis(gen)];
}