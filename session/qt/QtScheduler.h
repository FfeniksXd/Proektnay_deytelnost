#pragma once
#include "IScheduler.h"
#include <QElapsedTimer>
#include <QObject>
#include <cstdint>
#include <functional>
#include <map>

class QTimer;

// IScheduler поверх QTimer (см. контракт в IScheduler.h). Требует работающего цикла
// событий Qt. При уничтожении планировщика все его таймеры отменяются.
class QtScheduler : public IScheduler {
public:
    QtScheduler();
    ~QtScheduler() override;
    QtScheduler(const QtScheduler&) = delete;
    QtScheduler& operator=(const QtScheduler&) = delete;

    TimerId scheduleAfter(int ms, std::function<void()> callback) override;
    void cancel(TimerId id) override;
    std::int64_t nowMs() const override;

private:
    QObject context_;                  // владелец таймеров: уничтожается вместе с планировщиком
    QElapsedTimer clock_;
    std::map<TimerId, QTimer*> timers_;
    TimerId lastId_ = 0;
};