#include "QtScheduler.h"
#include <QTimer>
#include <utility>

QtScheduler::QtScheduler() {
    clock_.start();
}

QtScheduler::~QtScheduler() = default;

TimerId QtScheduler::scheduleAfter(int ms, std::function<void()> callback) {
    const TimerId id = ++lastId_;
    QTimer* timer = new QTimer(&context_);
    timer->setSingleShot(true);
    timers_[id] = timer;

    QObject::connect(timer, &QTimer::timeout, timer, [this, id, timer, cb = std::move(callback)] {
        timers_.erase(id);
        timer->deleteLater();   // нельзя удалять таймер внутри его же сигнала
        cb();
    });
    timer->start(ms);
    return id;
}

void QtScheduler::cancel(TimerId id) {
    auto it = timers_.find(id);
    if (it == timers_.end()) return;   // неизвестный или уже сработавший
    it->second->stop();
    it->second->deleteLater();
    timers_.erase(it);
}

std::int64_t QtScheduler::nowMs() const {
    return clock_.elapsed();
}