#pragma once
#include <cstdint>
#include <functional>

using TimerId = int;

// Часы и таймеры. Логика протокола (ping, пауза на переподключение) опирается на них,
// а не на QTimer напрямую: так в тестах время можно «перематывать» мгновенно.
//
// Контракт:
//  - scheduleAfter запускает функцию один раз через ms миллисекунд;
//    для повторов функция планирует себя заново.
//  - cancel отменяет ещё не сработавший таймер (неизвестный или уже сработавший id
//    игнорируется).
//  - Функции вызываются в том потоке, где живёт сессия.
class IScheduler {
public:
    virtual ~IScheduler() = default;

    virtual TimerId scheduleAfter(int ms, std::function<void()> callback) = 0;
    virtual void cancel(TimerId id) = 0;

    // Текущее время в миллисекундах (точка отсчёта произвольная)
    virtual std::int64_t nowMs() const = 0;
};