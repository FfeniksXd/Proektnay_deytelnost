#pragma once
#include "Board.h"
#include <optional>
#include <string>

// Интерфейс для GUI: сюда сессия отправляет события.
// Все события приходят в том потоке, где живёт сессия (для GUI это главный поток).
//
// Первые три метода обязательные. Остальные необязательные: у них пустая реализация
// по умолчанию, поэтому окно переопределяет только те, которые ему нужны,
// а добавление новых событий в будущем не ломает уже написанный код.
class IGameObserver {
public:
    virtual ~IGameObserver() = default;

    // ---- Обязательные ----

    // Состояние доски изменилось (ход, откат, новая партия)
    virtual void onBoardChanged(const Board& board) = 0;

    // Очередь перешла к игроку. После конца партии НЕ вызывается:
    // итог партии сообщает onGameOver.
    virtual void onTurnChanged(Cell currentPlayer) = 0;

    // Партия закончилась. winner пустой = ничья.
    virtual void onGameOver(std::optional<Cell> winner) = 0;

    // ---- Необязательные ----

    // Сессия ждёт ответ игрока (ИИ, модель, сеть) / дождалась.
    // Для показа «ИИ думает...». Для мгновенных ответов не вызывается.
    virtual void onThinkingChanged(Cell /*player*/, bool /*thinking*/) {}

    // Игрок (ИИ или сетевой) не смог дать допустимый ход, игра остановилась.
    virtual void onPlayerStalled(Cell /*player*/) {}

    // Соперник просит отменить его последний ход. Ответ: respondToUndo(...).
    virtual void onUndoRequested() {}
    // Запрос отмены отклонён (или устарел).
    virtual void onUndoDeclined() {}

    // Соперник просит начать партию заново. Ответ: respondToRestart(...).
    virtual void onRestartRequested() {}
    // Запрос перезапуска отклонён.
    virtual void onRestartDeclined() {}

    // Соединение с соперником потеряно, партия на паузе; осталось секунд ожидания.
    virtual void onOpponentDisconnected(int /*secondsLeft*/) {}
    // Соперник вернулся, партия продолжается.
    virtual void onOpponentReconnected() {}

    // Партия прервана без победителя (например, соперник не вернулся). reason: текст для показа.
    virtual void onGameAborted(const std::string& /*reason*/) {}

    // Прочие ошибки. code: короткий код (например "version_mismatch"), text: текст для показа.
    virtual void onError(const std::string& /*code*/, const std::string& /*text*/) {}
};