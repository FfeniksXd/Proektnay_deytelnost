#include "GameRules.h"

// Конструктор: сохраняем настройки игры
GameRules::GameRules(GameMode mode) : mode_(mode) {
    // ЗАЩИТА ОТ АБСУРДА: Нельзя выиграть, построив 1 или 0 в ряд
    if (mode_.winLength <= 1) {
        mode_.winLength = 3; 
    }
    // Если нужная длина больше поля, победа невозможна (обрабатывается в winner)
}

const GameMode& GameRules::mode() const {
    return mode_;
}

bool GameRules::isValidMove(const Board& board, Position p) const {
    // ЗАЩИТА: Проверяем границы до того, как обращаться к доске
    if (p.row < 0 || p.row >= board.size() || p.col < 0 || p.col >= board.size()) {
        return false;
    }
    // Ход валиден, только если клетка пуста
    return board.get(p.row, p.col) == Cell::Empty;
}

std::optional<Cell> GameRules::winner(const Board& board) const {
    int size = board.size();
    int winLen = mode_.winLength;
    
    if (winLen > size) return std::nullopt; 

    // Направления: Вправо, Вниз, Вправо-вниз (\), Влево-вниз (/)
    int dr[] = {0, 1, 1, 1};
    int dc[] = {1, 0, 1, -1};

    std::optional<Cell> lineBuilder = std::nullopt;

    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            Cell current = board.get(r, c);
            if (current == Cell::Empty) continue;

            for (int d = 0; d < 4; ++d) {
                int count = 1;
                for (int step = 1; step < winLen; ++step) {
                    int nr = r + dr[d] * step;
                    int nc = c + dc[d] * step;

                    if (nr < 0 || nr >= size || nc < 0 || nc >= size) break;

                    if (board.get(nr, nc) == current) {
                        count++;
                    } else {
                        break;
                    }
                }

                if (count >= winLen) {
                    lineBuilder = current;
                    break;
                }
            }
            if (lineBuilder.has_value()) break;
        }
        if (lineBuilder.has_value()) break;
    }

    if (!lineBuilder.has_value()) {
        return std::nullopt;
    }

    if (mode_.misere) {
        return (lineBuilder.value() == Cell::X) ? Cell::O : Cell::X;
    }

    return lineBuilder;
}

bool GameRules::isDraw(const Board& board) const {
    if (winner(board).has_value()) {
        return false;
    }

    for (int r = 0; r < board.size(); ++r) {
        for (int c = 0; c < board.size(); ++c) {
            if (board.get(r, c) == Cell::Empty) {
                return false; 
            }
        }
    }
    return true;
}

bool GameRules::isGameOver(const Board& board) const {
    return winner(board).has_value() || isDraw(board);
}