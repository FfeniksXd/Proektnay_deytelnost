#pragma once
#include "Board.h"
#include "GameRules.h"
#include <functional>
#include <optional>

// Колбэк, которым игрок сообщает выбранный ход (nullopt = «хода нет»)
using MoveCallback = std::function<void(std::optional<Position>)>;

// Правила для всех реализаций:
// 1. Не хранить ссылки board и rules после возврата из requestMove: к моменту
//    ответа они могут описывать уже другую позицию. Нужна доска позже - сделай копию.
// 2. onMove можно вызвать сразу (внутри requestMove) или позже, но не более одного раза
//    и только в том потоке, где живёт GameSession.
class IPlayer {
public:
    virtual ~IPlayer() = default;

    // Запросить ход. Игрок вызывает onMove, когда решит.
    virtual void requestMove(const Board& board, const GameRules& rules,
                             Cell self, MoveCallback onMove) = 0;

    // Позиция изменилась (откат, перезапуск): текущий запрос больше не нужен.
    // Можно прервать дорогую работу (запрос к модели, ожидание сети).
    // Не должен вызывать onMove.
    virtual void cancel() {}
};