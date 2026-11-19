#pragma once
#include "IPlayer.h"

// Сложный уровень (пока заглушка: ходит случайно).
// План: минимакс с альфа-бета отсечением, на больших полях с ограничением глубины.
class HardAI : public IPlayer {
public:
    void requestMove(const Board& board, const GameRules& rules,
                     Cell self, MoveCallback onMove) override;
};