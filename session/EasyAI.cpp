#include "EasyAI.h"
#include "AIUtils.h"

std::optional<Position> EasyAI::chooseMove(
    const Board& board, const GameRules& /*rules*/, Cell /*self*/) {
    return randomEmptyCell(board);
}