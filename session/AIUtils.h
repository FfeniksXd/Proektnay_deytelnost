#pragma once
#include "Board.h"
#include <optional>
#include <vector>

// Общие помощники для всех ИИ

// Все пустые клетки доски
std::vector<Position> emptyCells(const Board& board);

// Случайная пустая клетка (nullopt, если доска заполнена)
std::optional<Position> randomEmptyCell(const Board& board);