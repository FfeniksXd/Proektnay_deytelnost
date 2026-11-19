#include "HardAI.h"
#include "AIUtils.h"

void HardAI::requestMove(const Board& board, const GameRules& /*rules*/,
                         Cell /*self*/, MoveCallback onMove) {
    // TODO: минимакс (нужен рабочий GameRules::winner и учёт misere)
    onMove(randomEmptyCell(board));
}