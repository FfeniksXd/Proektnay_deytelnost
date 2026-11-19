#pragma once
#include "Board.h"
#include "GameRules.h"
#include <functional>
#include <optional>

// Колбэк с ответом игрока: выбранный ход или nullopt («хода нет»)
using MoveCallback = std::function<void(std::optional<Position>)>;

class IPlayer {
public:
    virtual ~IPlayer() = default;

    // Запросить ход. Игрок вызывает onMove, когда решит: сразу (внутри requestMove)
    // или позже. Правила для реализаций:
    //  1) НЕ хранить ссылки board и rules после возврата из requestMove:
    //     если они нужны позже, сделать КОПИЮ (Board копируется без проблем).
    //  2) Вызывать onMove в том же потоке, где живёт GameSession.
    //  3) Вызывать onMove не более одного раза на запрос.
    virtual void requestMove(const Board& board, const GameRules& rules,
                             Cell self, MoveCallback onMove) = 0;

    // Позиция изменилась (откат, перезапуск): текущий запрос больше не нужен.
    // Можно прервать дорогую работу (запрос к модели, ожидание сети).
    // Даже если игрок не отреагирует, сессия отбросит устаревший ответ сама.
    virtual void cancel() {}
};