#include "RemotePlayer.h"
#include <utility>

void RemotePlayer::requestMove(const Board& /*board*/, const GameRules& /*rules*/,
                               Cell /*self*/, MoveCallback onMove) {
    pending_ = std::move(onMove); // ответ придёт позже, из submitMove
}

void RemotePlayer::cancel() {
    pending_ = nullptr; // позиция изменилась, старый запрос не нужен
}

bool RemotePlayer::isWaiting() const {
    return static_cast<bool>(pending_);
}

bool RemotePlayer::submitMove(Position p) {
    if (!pending_) return false;
    MoveCallback callback = std::move(pending_);
    pending_ = nullptr;   // сбрасываем до вызова: колбэк может запросить ход заново
    callback(p);
    return true;
}