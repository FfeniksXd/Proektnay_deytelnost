#include "MediumAI.h"
#include "AIUtils.h"

void MediumAI::requestMove(const Board& board, const GameRules& /*rules*/,
                           Cell /*self*/, MoveCallback onMove) {
    // TODO: победный ход / блокировка (нужен рабочий GameRules::winner)
    onMove(randomEmptyCell(board));
}