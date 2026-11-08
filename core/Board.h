#pragma once
#include <vector>

enum class Cell { Empty, X, O };

struct Position {
    int row;
    int col;
};

class Board {
public:
    Board(int size = 3);
    int size() const;
    Cell get(int row, int col) const;
    void set(int row, int col, Cell value);
    bool isEmpty(int row, int col) const;
    void reset();
    
    // Возвращает плоский массив клеток (пригодится для тестов)
    std::vector<Cell> cells() const; 
private:
    int size_;
    std::vector<Cell> cells_;
};