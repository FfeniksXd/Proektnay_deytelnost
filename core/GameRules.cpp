#include "GameRules.h"

// Конструктор: сохраняем настройки игры (размер поля, длину победы, режим мизер)
GameRules::GameRules(GameMode mode) : mode_(mode) {}

// Возвращает текущие настройки игры
const GameMode& GameRules::mode() const {
    return mode_;
}

// Проверка, можно ли поставить фигуру в клетку (row, col)
bool GameRules::isValidMove(const Board& board, Position p) const {
    // Проверяем, не выходит ли ход за пределы поля
    if (p.row < 0 || p.row >= board.size() || p.col < 0 || p.col >= board.size()) {
        return false;
    }
    // Ход валиден, только если клетка пуста
    return board.get(p.row, p.col) == Cell::Empty;
}

// Главная функция: поиск победителя на всей доске
std::optional<Cell> GameRules::winner(const Board& board) const {
    int size = board.size();
    int winLen = mode_.winLength;
    
    // Если нужная длина линии больше размера поля, победа невозможна
    if (winLen > size) return std::nullopt; 

    // Массивы смещений для 4 направлений: 
    // 0: Вправо, 1: Вниз, 2: Вправо-вниз (\), 3: Влево-вниз (/)
    int dr[] = {0, 1, 1, 1};
    int dc[] = {1, 0, 1, -1};

    // Здесь будет храниться символ того, кто построил линию (X или O)
    std::optional<Cell> lineBuilder = std::nullopt;

    // Проходим по каждой клетке поля (сканируем сверху-вниз, слева-направо)
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            Cell current = board.get(r, c);
            
            // Пустая клетка не может быть началом линии
            if (current == Cell::Empty) continue;

            // Проверяем все 4 направления из текущей клетки
            for (int d = 0; d < 4; ++d) {
                int count = 1; // Текущая клетка уже считается (1)
                
                // Шагаем вперед по выбранному направлению
                for (int step = 1; step < winLen; ++step) {
                    int nr = r + dr[d] * step; // Вычисляем следующую строку
                    int nc = c + dc[d] * step; // Вычисляем следующую колонку

                    // Если вышли за границы поля — прерываем проверку этого направления
                    if (nr < 0 || nr >= size || nc < 0 || nc >= size) break;

                    // Если следующая клетка совпадает с текущей — увеличиваем счет
                    if (board.get(nr, nc) == current) {
                        count++;
                    } else {
                        break; // Линия прервалась (встретили врага или пустоту)
                    }
                }

                // Если набрали нужное количество в ряд
                if (count >= winLen) {
                    lineBuilder = current; // Запоминаем, кто выиграл
                    break; // Выходим из цикла направлений
                }
            }
            if (lineBuilder.has_value()) break; // Выходим из цикла колонок
        }
        if (lineBuilder.has_value()) break; // Выходим из цикла строк
    }

    // Если никто не построил линию
    if (!lineBuilder.has_value()) {
        return std::nullopt;
    }

    // Логика MISERE (Мизер)
    if (mode_.misere) {
        // В мизере тот, кто построил линию, ПРОИГРАЛ. Возвращаем соперника.
        return (lineBuilder.value() == Cell::X) ? Cell::O : Cell::X;
    }

    // Обычный режим: победил тот, кто построил
    return lineBuilder;
}

// Проверка на ничью
bool GameRules::isDraw(const Board& board) const {
    // Если есть победитель — это не ничья
    if (winner(board).has_value()) {
        return false;
    }

    // Ищем хотя бы одну пустую клетку
    for (int r = 0; r < board.size(); ++r) {
        for (int c = 0; c < board.size(); ++c) {
            if (board.get(r, c) == Cell::Empty) {
                return false; // Нашли пустую клетку — игра еще идет
            }
        }
    }
    // Доска полна и победителя нет
    return true;
}

// Закончилась ли игра (победа или ничья)
bool GameRules::isGameOver(const Board& board) const {
    return winner(board).has_value() || isDraw(board);
}