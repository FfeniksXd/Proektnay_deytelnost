#pragma once
#include "IPlayer.h"

// Средний уровень (пока заглушка: ходит случайно).
// План: 1) если можно выиграть этим ходом - выиграть;
//       2) если соперник выигрывает следующим ходом - заблокировать;
//       3) иначе случайный ход.
class MediumAI : public IPlayer {
public:
    std::optional<Position> chooseMove(
        const Board& board, const GameRules& rules, Cell self) override;
};