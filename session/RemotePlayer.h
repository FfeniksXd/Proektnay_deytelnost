#pragma once
#include "IPlayer.h"

// Удалённый игрок на стороне хоста: «почтовый ящик» для ходов, пришедших по сети.
// Сам сети не знает: ходы ему передаёт NetworkHost через submitMove().
//
// requestMove только запоминает, что сессия ждёт ход. submitMove отдаёт пришедший
// ход сессии, но только если она действительно ждёт; иначе возвращает false
// (хост в этом случае отвечает клиенту move_rejected / not_your_turn).
// Допустимость клетки проверяет хост ДО submitMove: недопустимый ход, отданный сессии,
// остановил бы партию (см. stalled_ в GameSession).
class RemotePlayer : public IPlayer {
public:
    void requestMove(const Board& board, const GameRules& rules,
                     Cell self, MoveCallback onMove) override;
    void cancel() override;

    bool isWaiting() const;          // сессия ждёт ход этого игрока
    bool submitMove(Position p);     // true, если ход передан сессии

private:
    MoveCallback pending_;
};