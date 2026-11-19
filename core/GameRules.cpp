#include "GameRules.h"

using namespace std;

// Конструктор класса. Принимает все настройки игры
// и сохраняет их в приватное поле mode_ для дальнейшего использования.
GameRules::GameRules(GameMode mode) : mode_(mode) {}

// Получатель настроек. Возвращает константную ссылку на режим игры.
const GameMode& GameRules::mode() const{
    return mode_;
}

// Проверка, можно ли сделать ход в указанную клетку
bool GameRules::isValidMove(const Board& board, Position p) const{
    // Проверка на выход за границы игрового поля.
    if(p.row < 0 || p.row >= board.size() || p.col < 0 || p.col >= board.size()){
        return false;
    }
    // Проверка пуста ли клетка.
    return board.get(p.row, p.col) == Cell::Empty;
}

// Главная функция проверки победителя
optional<Cell> GameRules::winner(const Board& board) const {
    int size = board.size();            // Получаем размер поля
    int winLen = mode_.winLength;       // Получаем длинну для победы


    if(winLen > size) return nullopt; // Защита от дурака: если нужно собрать 4 в ряд на поле 3х3, это невозможно.

    // Массивы смещений для 4 направлений. 
    // dr = изменение строки (row), dc = изменение колонки (col).
    // 1. {0, 1}  - Вправо (строка та же, колонка +1)
    // 2. {1, 0}  - Вниз (строка +1, колонка та же)
    // 3. {1, 1}  - Вправо-вниз \ (строка +1, колонка +1)
    // 4. {1, -1} - Влево-вниз / (строка +1, колонка -1)
    int dr[] = {0,1,1,1};
    int dc[] = {1,0,1,-1};


    optional<Cell> lineBuilder = nullopt;   // Переменная для хранения того, кто построил линию

    // Проходим по каждой строке поля
    for (int r = 0; r < size; r++){
        // Проходим по каждой колонке в текущей строке
        for (int c = 0; c < size; ++c){
            // Берем текущую клетку
            Cell current = board.get(r, c);

            // Если клетка пустая, она не может быть началом линии. Пропускаем её.
            if (current == Cell::Empty) continue;
            
            // Если клетка занята, проверяем все 4 направления из неё
            for (int d = 0; d < 4; ++d){
                int count = 1;// Счетчик. Текущая клетка уже дает 1 в копилку

                for (int step = 1; step < winLen; ++step){
                    // Вычисляем координаты следующей клетки.(next row, next col)
                    int nr = r + dr[d] * step;
                    int nc = c + dc[d] * step;

                    // Если следующая клетка выходит за границы поля — прерываем проверку этого направления
                    if (nr < 0 || nr > size || nc < 0 || nc > size) break;

                    // Если следующая клетка принадлежит тому же игроку (X или O) — увеличиваем счет
                    if (board.get(nr, nc) == current){
                        count++;
                    }
                    else{
                        break; // Если встретили пустую клетку или врага — линия прервалась, выходим
                    }
                }
                // Если после всех шагов счетчик достиг нужной длины
                if (count >= winLen){
                    lineBuilder = current;  // Запоминаем, кто построил линию
                    break;
                }
            }
            // Если линия была найдена, нет смысла проверять остальные клетки доски. Выходим.
            if(lineBuilder.has_value()) break;
        }
        // Повторная проверка выхода из внешнего цикла
        if(lineBuilder.has_value()) break;
    }

    // Если никто не построил линию, возвращаем пустоту (nullopt)
    if(!lineBuilder.has_value()){
        return nullopt;
    }
    // Логика MISERE (Мизер)
    if(mode_.misere){
        // В мизере тот, кто построил линию, ПРОИГРЫВАЕТ. 
        // Значит, победителем должен быть его соперник.
        // Если линию построил X, возвращаем O. Если O, возвращаем X.
        return(lineBuilder.value() == Cell::X) ? Cell::O : Cell::X;
    }
    // В обычном режиме победил тот, кто построил линию
    return lineBuilder;
}

// Проверка на ничью
bool GameRules::isDraw(const Board& board) const{
    // Если есть победитель — это по определению не ничья
    if(winner(board).has_value()){
        return false;
    }
    // Проходим по всей доске в поисках пустых клеток
    for(int r = 0; r < board.size(); r++){
        for(int c = 0; c < board.size(); ++c){
            // Если найдена хоть одна пустая клетка игра еще не закончена.
            if(board.get(r, c) == Cell::Empty){
                return false;
            }
        }
    }
    //  Если мы дошли сюда: победителя нет и пустых клеток нет.
    // Значит, доска заполнена, и это ничья.
    return true;
}

// Проверка, закончилась ли игра.
bool GameRules::isGameOver(const Board& board) const{
    return winner(board).has_value() || isDraw(board);
}