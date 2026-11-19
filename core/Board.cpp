#include "Board.h"

// Конструктор: создаем доску нужного размера и заполняем её пустыми клетками
Board::Board(int size) : size_(size), cells_(size * size, Cell::Empty) {}

// Возвращаем размер доски (3 для 3х3 и т.д.)
int Board::size() const {
    return size_;
}