#pragma once
// Часы для тестов: время стоит на месте, пока тест сам не вызовет advance().
// Так «60 секунд ожидания» в тесте занимают микросекунды.
#include "IScheduler.h"
#include <cstddef>
#include <functional>
#include <map>
#include <utility>

class FakeScheduler : public IScheduler {
public:
    TimerId scheduleAfter(int ms, std::function<void()> callback) override {
        const TimerId id = ++lastId_;
        timers_.emplace(std::make_pair(now_ + ms, id), std::move(callback));
        return id;
    }

    void cancel(TimerId id) override {
        for (auto it = timers_.begin(); it != timers_.end(); ++it) {
            if (it->first.second == id) {
                timers_.erase(it);
                return;
            }
        }
    }

    std::int64_t nowMs() const override { return now_; }

    // Перемотать время вперёд, выполняя сработавшие таймеры по порядку.
    // Таймеры, запланированные обработчиками, тоже сработают, если попадают в интервал.
    // (Таймер, который планирует себя с ms = 0, зациклит тест, так делать не надо.)
    void advance(int ms) {
        const std::int64_t target = now_ + ms;
        while (!timers_.empty() && timers_.begin()->first.first <= target) {
            auto it = timers_.begin();
            now_ = it->first.first;
            auto callback = std::move(it->second);
            timers_.erase(it);
            callback();
        }
        now_ = target;
    }

    std::size_t pending() const { return timers_.size(); }

private:
    std::int64_t now_ = 0;
    TimerId lastId_ = 0;
    std::map<std::pair<std::int64_t, TimerId>, std::function<void()>> timers_;
};