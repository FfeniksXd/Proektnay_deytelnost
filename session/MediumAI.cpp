#include "MediumAI.h"
#include "AIUtils.h"

std::optional<Position> MediumAI::chooseMove(
    const Board& board, const GameRules& /*rules*/, Cell /*self*/) {
    // TODO: победный ход / блокировка (нужен рабочий GameRules::winner)
    return randomEmptyCell(board);
}