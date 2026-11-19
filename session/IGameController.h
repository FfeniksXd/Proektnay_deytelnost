#pragma once
#include "Board.h"
#include "IGameObserver.h"
#include <optional>

// Интерфейс управления партией: то, что видит GUI.
// Его реализуют локальная сессия (GameSession) и, позже, сетевой клиент:
// окно работает с ними одинаково и не знает, где ведётся игра.
class IGameController {
public:
    virtual ~IGameController() = default;

    // Окно, которому будут приходить события (может быть nullptr)
    virtual void setObserver(IGameObserver* observer) = 0;

    // Начать партию (или начать заново)
    virtual void start() = 0;

    // Ход человека (клик по клетке). false = ход не принят.
    virtual bool makeHumanMove(Position p) = 0;

    // Просьба отменить ход. В локальной игре выполняется сразу (результат придёт
    // через onBoardChanged), в сетевой нужно согласие соперника.
    virtual void requestUndo() = 0;
    // Ответ на запрос отмены от соперника (см. onUndoRequested)
    virtual void respondToUndo(bool accept) = 0;

    // Просьба начать партию заново; в сетевой игре нужно согласие соперника
    virtual void requestRestart() = 0;
    // Ответ на запрос перезапуска от соперника (см. onRestartRequested)
    virtual void respondToRestart(bool accept) = 0;

    // Текущее состояние (для первичной отрисовки окна)
    virtual const Board& board() const = 0;
    virtual Cell currentPlayer() const = 0;
    virtual bool isGameOver() const = 0;
    virtual std::optional<Cell> winner() const = 0;
};