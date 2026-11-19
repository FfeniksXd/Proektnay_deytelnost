#include <gtest/gtest.h>
#include "GameSession.h"
#include "RemotePlayer.h"
#include <memory>
#include <utility>
#include <vector>

namespace {

const GameMode kClassic{"Classic", 3, 3, false};

// Тестовый игрок: отвечает, когда тест сам вызовет callback.
// cancel() намеренно НЕ сбрасывает колбэк: так имитируем плохо ведущего себя игрока,
// чей ответ приходит уже после отката (сессия обязана это пережить).
class ManualPlayer : public IPlayer {
public:
    void requestMove(const Board&, const GameRules&, Cell, MoveCallback onMove) override {
        callback = std::move(onMove);
        ++requests;
    }
    void cancel() override { ++cancels; }

    MoveCallback callback;
    int requests = 0;
    int cancels = 0;
};

struct Recorder : IGameObserver {
    std::vector<std::pair<Cell, bool>> thinking;
    std::vector<Cell> stalled;
    void onBoardChanged(const Board&) override {}
    void onTurnChanged(Cell) override {}
    void onGameOver(std::optional<Cell>) override {}
    void onThinkingChanged(Cell player, bool isThinking) override {
        thinking.push_back({player, isThinking});
    }
    void onPlayerStalled(Cell player) override { stalled.push_back(player); }
};

int filled(const Board& b) {
    int n = 0;
    for (int r = 0; r < b.size(); ++r)
        for (int c = 0; c < b.size(); ++c)
            if (!b.isEmpty(r, c)) ++n;
    return n;
}

} // namespace

TEST(AsyncSessionTest, StaleAnswerAfterUndoIsDiscarded) {
    auto manual = std::make_unique<ManualPlayer>();
    ManualPlayer* m = manual.get();
    GameSession s(kClassic, nullptr, std::move(manual)); // X - человек, O - ManualPlayer
    s.start();

    ASSERT_TRUE(s.makeHumanMove({0, 0}));
    EXPECT_EQ(m->requests, 1);
    MoveCallback stale = m->callback;      // «ответ, который придёт позже»

    const int cancelsBefore = m->cancels;   // start() тоже вызывает cancel(), считаем разницу
    s.requestUndo();                        // отмена, пока соперник «думает»
    EXPECT_EQ(filled(s.board()), 0);
    EXPECT_EQ(s.currentPlayer(), Cell::X);
    EXPECT_EQ(m->cancels, cancelsBefore + 1); // игроку сообщили, что запрос больше не нужен

    stale(Position{1, 1});                  // запоздалый ответ на уже не существующую позицию
    EXPECT_EQ(filled(s.board()), 0);        // отброшен: ход не поставлен
    EXPECT_EQ(s.currentPlayer(), Cell::X);  // и игра не остановилась
    EXPECT_TRUE(s.makeHumanMove({2, 2}));   // человек продолжает играть
}

TEST(AsyncSessionTest, ThinkingEventsForSlowPlayer) {
    auto manual = std::make_unique<ManualPlayer>();
    ManualPlayer* m = manual.get();
    GameSession s(kClassic, nullptr, std::move(manual));
    Recorder rec;
    s.setObserver(&rec);
    s.start();

    s.makeHumanMove({0, 0});
    ASSERT_EQ(rec.thinking.size(), 1u);
    EXPECT_EQ(rec.thinking[0].first, Cell::O);
    EXPECT_TRUE(rec.thinking[0].second);

    m->callback(Position{1, 1});            // игрок наконец ответил
    ASSERT_EQ(rec.thinking.size(), 2u);
    EXPECT_FALSE(rec.thinking[1].second);
    EXPECT_FALSE(s.board().isEmpty(1, 1));
    EXPECT_EQ(s.currentPlayer(), Cell::X);
}

TEST(AsyncSessionTest, InvalidAnswerStallsInsteadOfHanging) {
    auto manual = std::make_unique<ManualPlayer>();
    ManualPlayer* m = manual.get();
    GameSession s(kClassic, nullptr, std::move(manual));
    Recorder rec;
    s.setObserver(&rec);
    s.start();

    s.makeHumanMove({0, 0});
    m->callback(Position{0, 0});            // клетка занята

    ASSERT_EQ(rec.stalled.size(), 1u);
    EXPECT_EQ(rec.stalled[0], Cell::O);
    EXPECT_EQ(filled(s.board()), 1);        // доска не испорчена
}

TEST(RemotePlayerTest, DeliversNetworkMoveToSession) {
    auto remote = std::make_unique<RemotePlayer>();
    RemotePlayer* r = remote.get();
    GameSession s(kClassic, nullptr, std::move(remote)); // O - удалённый игрок
    s.start();

    EXPECT_FALSE(r->isWaiting());           // сейчас ход человека
    EXPECT_FALSE(r->submitMove({1, 1}));    // не его очередь: ход не принят

    ASSERT_TRUE(s.makeHumanMove({0, 0}));
    EXPECT_TRUE(r->isWaiting());            // теперь сессия ждёт удалённого игрока

    EXPECT_TRUE(r->submitMove({1, 1}));
    EXPECT_FALSE(r->isWaiting());
    EXPECT_FALSE(s.board().isEmpty(1, 1));
    EXPECT_EQ(s.currentPlayer(), Cell::X);
    EXPECT_FALSE(r->submitMove({2, 2}));    // второй ход подряд не принимается
}

TEST(RemotePlayerTest, UndoCancelsPendingRequest) {
    auto remote = std::make_unique<RemotePlayer>();
    RemotePlayer* r = remote.get();
    GameSession s(kClassic, nullptr, std::move(remote));
    s.start();
    s.makeHumanMove({0, 0});
    ASSERT_TRUE(r->isWaiting());

    s.requestUndo();
    EXPECT_FALSE(r->isWaiting());           // запрос сброшен
    EXPECT_FALSE(r->submitMove({1, 1}));    // и опоздавший сетевой ход не пройдёт
}