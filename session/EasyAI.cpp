#include "EasyAI.h"
#include "AIUtils.h"

void EasyAI::requestMove(const Board& board, const GameRules& /*rules*/,
                         Cell /*self*/, MoveCallback onMove) {
    onMove(randomEmptyCell(board)); // ответ приходит сразу, внутри вызова
}