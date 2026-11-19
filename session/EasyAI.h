#pragma once
#include "IPlayer.h"

// Лёгкий уровень: случайный ход среди пустых клеток
class EasyAI : public IPlayer {
public:
    void requestMove(const Board& board, const GameRules& rules,
                     Cell self, MoveCallback onMove) override;
};