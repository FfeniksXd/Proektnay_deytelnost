#include "HardAI.h"
#include "AIUtils.h"

std::optional<Position> HardAI::chooseMove(
    const Board& board, const GameRules& /*rules*/, Cell /*self*/) {
    // TODO: минимакс (нужен рабочий GameRules::winner и учёт misere)
    return randomEmptyCell(board);
}