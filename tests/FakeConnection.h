#pragma once
// Заглушка сети для тестов: «провод» между двумя концами в памяти.
// Строки не доставляются сразу, а копятся в очереди и доходят при вызове deliverAll():
// так поведение похоже на реальную сеть (ответ приходит не внутри send) и предсказуемо.
#include "IConnection.h"
#include <deque>
#include <functional>
#include <string>
#include <utility>

class FakeLink {
public:
    FakeLink() : a_(*this, 0), b_(*this, 1) {}
    FakeLink(const FakeLink&) = delete;
    FakeLink& operator=(const FakeLink&) = delete;

    IConnection& a() { return a_; }
    IConnection& b() { return b_; }

    // Доставить всё накопленное, включая то, что породят обработчики.
    // Возвращает число доставленных событий (limit защищает от бесконечного обмена).
    int deliverAll(int limit = 10000) {
        int n = 0;
        while (!queue_.empty() && n < limit) {
            Event e = std::move(queue_.front());
            queue_.pop_front();
            ++n;
            Endpoint& ep = (e.to == 0) ? a_ : b_;
            if (e.closed) {
                if (!ep.finished_) {
                    ep.finished_ = true;
                    ep.open_ = false;
                    if (ep.onClosed_) ep.onClosed_();
                }
            } else if (ep.open_ && ep.onLine_) {
                ep.onLine_(e.line);
            }
        }
        return n;
    }

    // Обрыв с уведомлением обоих концов (как закрытый сокет)
    void cut() {
        queue_.push_back({0, true, {}});
        queue_.push_back({1, true, {}});
    }

    // Тихая потеря: пока включено, отправленные строки пропадают и никто ничего
    // не узнаёт (как выдернутый кабель). Проверяет ping и таймеры тишины.
    void setSilentDrop(bool on) { drop_ = on; }

    bool hasPending() const { return !queue_.empty(); }

private:
    struct Event {
        int to;
        bool closed;
        std::string line;
    };

    class Endpoint : public IConnection {
    public:
        Endpoint(FakeLink& link, int index) : link_(link), index_(index) {}

        void send(const std::string& line) override {
            if (!open_ || link_.drop_) return;
            link_.queue_.push_back({1 - index_, false, line});
        }
        void close() override {
            if (!open_) return;
            open_ = false;
            finished_ = true; // о своём закрытии не уведомляем
            link_.queue_.push_back({1 - index_, true, {}});
        }
        bool isOpen() const override { return open_; }
        void setOnLine(std::function<void(const std::string&)> cb) override { onLine_ = std::move(cb); }
        void setOnClosed(std::function<void()> cb) override { onClosed_ = std::move(cb); }

    private:
        friend class FakeLink;
        FakeLink& link_;
        int index_;
        bool open_ = true;
        bool finished_ = false;
        std::function<void(const std::string&)> onLine_;
        std::function<void()> onClosed_;
    };

    std::deque<Event> queue_;
    bool drop_ = false;
    Endpoint a_;
    Endpoint b_;
};