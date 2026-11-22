#include "Board.h"

// Конструктор: создаем доску нужного размера (например, 3х3)
Board::Board(int size) 
    : size_(size),                       // Сохраняем размер поля
      cells_(size * size, Cell::Empty)   // Создаем одномерный массив и заполняем пустотой
{
    // Всё сделано в списке инициализации
}

// Возвращает размер доски (3 для 3х3)
int Board::size() const {
    return size_;
}

// Получить значение клетки по координатам (row, col)
Cell Board::get(int row, int col) const {
    // Используем математику для перевода 2D координат в 1D индекс
    // Формула: (строка * размер_доски) + колонка
    return cells_[row * size_ + col];
}

// Установить значение в клетку (row, col)
void Board::set(int row, int col, Cell value) {
    // Используем ту же формулу, чтобы найти нужную ячейку в памяти
    cells_[row * size_ + col] = value;
}

// Проверить, пуста ли клетка
bool Board::isEmpty(int row, int col) const {
    // Ищем клетку по индексу и сравниваем с Cell::Empty
    return cells_[row * size_ + col] == Cell::Empty;
}

// Очистить доску (используется при рестарте игры)
void Board::reset() {
    // Метод assign перезаписывает весь вектор заново
    cells_.assign(size_ * size_, Cell::Empty);
}

// Вернуть копию всего одномерного массива клеток (нужно для сети и тестов)
std::vector<Cell> Board::cells() const {
    return cells_;
}