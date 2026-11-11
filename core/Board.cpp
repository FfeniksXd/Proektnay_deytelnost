#include "Board.h"

Board::Board(int size) : size_(size), cells_(size * size, Cell::Empty) {}

int Board::size() const { return size_; }

Cell Board::get(int row, int col) const {
    return cells_[row * size_ + col];
}

void Board::set(int row, int col, Cell value) {
    cells_[row * size_ + col] = value;
}

bool Board::isEmpty(int row, int col) const {
    return get(row, col) == Cell::Empty;
}

void Board::reset() {
    cells_.assign(size_ * size_, Cell::Empty);
}

std::vector<Cell> Board::cells() const {
    return cells_;
}