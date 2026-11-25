#include "Board.h"

Board::Board(int size) : size_(size), cells_(size * size, Cell::Empty) {}

int Board::size() const {
    return size_;
}

Cell Board::get(int row, int col) const {
    // ЗАЩИТА: Если координаты за пределами поля, возвращаем Empty (или можно бросать исключение)
    if (row < 0 || row >= size_ || col < 0 || col >= size_) {
        return Cell::Empty;
    }
    return cells_[row * size_ + col];
}

void Board::set(int row, int col, Cell value) {
    // ЗАЩИТА: Игнорируем попытку записи за пределами поля
    if (row < 0 || row >= size_ || col < 0 || col >= size_) {
        return; 
    }
    cells_[row * size_ + col] = value;
}

bool Board::isEmpty(int row, int col) const {
    // ЗАЩИТА: Клетка за пределами поля не может быть пустой (для хода)
    if (row < 0 || row >= size_ || col < 0 || col >= size_) {
        return false;
    }
    return cells_[row * size_ + col] == Cell::Empty;
}

void Board::reset() {
    cells_.assign(size_ * size_, Cell::Empty);
}

std::vector<Cell> Board::cells() const {
    return cells_;
}