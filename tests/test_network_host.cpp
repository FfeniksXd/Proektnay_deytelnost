#include <gtest/gtest.h>
#include "FakeConnection.h"
#include "NetworkHost.h"
#include <initializer_list>
#include <string>
#include <vector>

using namespace proto;

namespace {

const GameMode kClassic{"Classic", 3, 3, false};

// «Клиент» на другом конце провода: собирает всё, что прислал хост
class TestClient {
public:
    explicit TestClient(IConnection& c) : conn_(c) {
        conn_.setOnLine([this](const std::string& line) {
            DecodeResult r = decode(line);
            if (r.message) inbox.push_back(*r.message);
            else undecodable.push_back(line);
        });
        conn_.setOnClosed([this] { closed = true; });
    }
    void send(const Message& m) { conn_.send(encode(m)); }
    void sendRaw(const std::string& s) { conn_.send(s); }

    template <class T> const T* last() const {   // последнее сообщение типа T
        for (auto it = inbox.rbegin(); it != inbox.rend(); ++it) {
            if (const T* p = std::get_if<T>(&*it)) return p;
        }
        return nullptr;
    }
    template <class T> int count() const {
        int n = 0;
        for (const Message& m : inbox) if (std::holds_alternative<T>(m)) ++n;
        return n;
    }

    std::vector<Message> inbox;
    std::vector<std::string> undecodable;
    bool closed = false;

private:
    IConnection& conn_;
};

// Окно хоста: запоминает, что ему сообщили
struct GuiRecorder : IGameObserver {
    int aborted = 0;
    std::string abortReason;
    int moves = 0;
    void onBoardChanged(const Board&) override {}
    void onTurnChanged(Cell) override {}
    void onGameOver(std::optional<Cell>) override {}
    void onMoveApplied(const MoveRecord&, int) override { ++moves; }
    void onGameAborted(const std::string& reason) override { ++aborted; abortReason = reason; }
};

HostOptions hostOptions(HostSide side, const std::string& token = "T1") {
    HostOptions o;
    o.side = side;
    o.makeToken = [token] { return token; };
    return o;
}

// Доставлять, пока по любому из проводов что-то идёт
void pump(std::initializer_list<FakeLink*> links) {
    for (int round = 0; round < 100; ++round) {
        bool any = false;
        for (FakeLink* l : links) {
            if (l->hasPending()) { l->deliverAll(); any = true; }
        }
        if (!any) return;
    }
}

int filled(const Board& b) {
    int n = 0;
    for (int r = 0; r < b.size(); ++r)
        for (int c = 0; c < b.size(); ++c)
            if (!b.isEmpty(r, c)) ++n;
    return n;
}

// Подключить клиента и дойти до начала партии
void connect(NetworkHost& host, FakeLink& link, TestClient& client) {
    host.acceptConnection(link.a());
    client.send(Hello{});
    pump({&link});
}

} // namespace

TEST(NetworkHostTest, HandshakeSendsWelcomeAndStartsGame) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X, "T1"));
    TestClient client(link.b());
    EXPECT_EQ(host.state(), NetworkHost::State::WaitingForClient);

    connect(host, link, client);

    EXPECT_EQ(host.state(), NetworkHost::State::Playing);
    const Welcome* w = client.last<Welcome>();
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->protocol, kProtocolVersion);
    EXPECT_EQ(w->yourSymbol, Cell::O);          // хост выбрал X
    EXPECT_EQ(w->resumeToken, "T1");
    EXPECT_EQ(w->mode.name, "Classic");
    EXPECT_EQ(w->mode.boardSize, 3);
}

TEST(NetworkHostTest, WelcomeComesBeforeAnyMove) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::O)); // клиент ходит первым (X)
    TestClient client(link.b());
    connect(host, link, client);
    ASSERT_FALSE(client.inbox.empty());
    EXPECT_TRUE(std::holds_alternative<Welcome>(client.inbox[0]));
}

TEST(NetworkHostTest, RandomSideUsesCoinFlip) {
    FakeLink link;
    HostOptions o;
    o.side = HostSide::Random;
    o.coinFlip = [] { return false; };          // false = хост играет за O
    NetworkHost host(kClassic, o);
    TestClient client(link.b());
    connect(host, link, client);

    EXPECT_EQ(client.last<Welcome>()->yourSymbol, Cell::X);
}

TEST(NetworkHostTest, ClientMovesFirstWhenItPlaysX) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::O));
    TestClient client(link.b());
    connect(host, link, client);

    client.send(Move{{0, 0}});
    pump({&link});

    EXPECT_FALSE(host.board().isEmpty(0, 0));
    const MoveApplied* ma = client.last<MoveApplied>();
    ASSERT_NE(ma, nullptr);
    EXPECT_EQ(ma->moveNo, 1);
    EXPECT_EQ(ma->player, Cell::X);
    EXPECT_EQ(ma->pos.row, 0);
    EXPECT_EQ(ma->pos.col, 0);
    EXPECT_EQ(host.currentPlayer(), Cell::O);   // теперь ход хоста
}

TEST(NetworkHostTest, HostMoveIsSentToClientAndClientReplies) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    TestClient client(link.b());
    connect(host, link, client);

    EXPECT_TRUE(host.makeHumanMove({1, 1}));
    pump({&link});
    const MoveApplied* first = client.last<MoveApplied>();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->moveNo, 1);
    EXPECT_EQ(first->player, Cell::X);

    client.send(Move{{0, 0}});
    pump({&link});
    EXPECT_EQ(client.count<MoveApplied>(), 2);
    EXPECT_EQ(client.last<MoveApplied>()->player, Cell::O);
    EXPECT_FALSE(host.board().isEmpty(0, 0));
}

TEST(NetworkHostTest, MoveOutOfTurnIsRejected) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));   // сейчас ход хоста
    TestClient client(link.b());
    connect(host, link, client);

    client.send(Move{{0, 0}});
    pump({&link});

    const MoveRejected* rej = client.last<MoveRejected>();
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::NotYourTurn);
    EXPECT_EQ(filled(host.board()), 0);
}

TEST(NetworkHostTest, OccupiedCellIsRejectedAndGameKeepsGoing) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    TestClient client(link.b());
    connect(host, link, client);
    host.makeHumanMove({1, 1});
    pump({&link});

    client.send(Move{{1, 1}});                  // клетка занята
    pump({&link});
    ASSERT_NE(client.last<MoveRejected>(), nullptr);
    EXPECT_EQ(client.last<MoveRejected>()->reason, RejectReason::InvalidCell);

    client.send(Move{{0, 0}});                  // партия не остановилась: нормальный ход проходит
    pump({&link});
    EXPECT_FALSE(host.board().isEmpty(0, 0));
}

TEST(NetworkHostTest, WrongVersionIsRefusedAndHostKeepsWaiting) {
    FakeLink bad, good;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    TestClient badClient(bad.b()), goodClient(good.b());

    host.acceptConnection(bad.a());
    Hello hello;
    hello.protocol = 99;
    badClient.send(hello);
    pump({&bad});

    ASSERT_NE(badClient.last<Error>(), nullptr);
    EXPECT_EQ(badClient.last<Error>()->code, "version_mismatch");
    EXPECT_TRUE(badClient.closed);
    EXPECT_EQ(host.state(), NetworkHost::State::WaitingForClient);

    connect(host, good, goodClient);            // хост по-прежнему принимает нормальных клиентов
    EXPECT_EQ(host.state(), NetworkHost::State::Playing);
}

TEST(NetworkHostTest, FirstMessageMustBeHello) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    TestClient client(link.b());
    host.acceptConnection(link.a());

    client.send(Move{{0, 0}});
    pump({&link});

    ASSERT_NE(client.last<Error>(), nullptr);
    EXPECT_EQ(client.last<Error>()->code, "unexpected_message");
    EXPECT_TRUE(client.closed);
    EXPECT_EQ(host.state(), NetworkHost::State::WaitingForClient);
}

TEST(NetworkHostTest, ResumeTokenWithoutGameIsRefused) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    TestClient client(link.b());
    host.acceptConnection(link.a());

    Hello hello;
    hello.resumeToken = "T1";
    client.send(hello);
    pump({&link});

    ASSERT_NE(client.last<Error>(), nullptr);
    EXPECT_EQ(client.last<Error>()->code, "unknown_session");
}

TEST(NetworkHostTest, SecondClientIsRejectedWhileGameInProgress) {
    FakeLink first, second;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    TestClient c1(first.b()), c2(second.b());
    connect(host, first, c1);

    host.acceptConnection(second.a());
    c2.send(Hello{});                           // без токена
    pump({&first, &second});

    ASSERT_NE(c2.last<Error>(), nullptr);
    EXPECT_EQ(c2.last<Error>()->code, "game_in_progress");
    EXPECT_TRUE(c2.closed);

    EXPECT_FALSE(c1.closed);                    // партия идёт как шла
    EXPECT_EQ(host.state(), NetworkHost::State::Playing);
    host.makeHumanMove({1, 1});
    pump({&first, &second});
    EXPECT_EQ(c1.count<MoveApplied>(), 1);
}

TEST(NetworkHostTest, WrongTokenDoesNotStealTheGame) {
    FakeLink first, second;
    NetworkHost host(kClassic, hostOptions(HostSide::X, "T1"));
    TestClient c1(first.b()), c2(second.b());
    connect(host, first, c1);

    host.acceptConnection(second.a());
    Hello hello;
    hello.resumeToken = "WRONG";
    c2.send(hello);
    pump({&first, &second});

    ASSERT_NE(c2.last<Error>(), nullptr);
    EXPECT_EQ(c2.last<Error>()->code, "game_in_progress");
    EXPECT_FALSE(c1.closed);
}

TEST(NetworkHostTest, ValidTokenReplacesOldConnection) {
    FakeLink first, second;
    NetworkHost host(kClassic, hostOptions(HostSide::X, "T1"));
    TestClient c1(first.b()), c2(second.b());
    connect(host, first, c1);
    host.makeHumanMove({1, 1});
    pump({&first});

    // Клиент вернулся с новым соединением, хост ещё не заметил, что старое пропало
    host.acceptConnection(second.a());
    Hello hello;
    hello.resumeToken = "T1";
    c2.send(hello);
    pump({&first, &second});

    ASSERT_GE(c2.inbox.size(), 2u);
    EXPECT_TRUE(std::holds_alternative<Welcome>(c2.inbox[0]));
    EXPECT_TRUE(std::holds_alternative<Sync>(c2.inbox[1]));
    const Sync* sync = c2.last<Sync>();
    EXPECT_EQ(sync->moveCount, 1);
    EXPECT_EQ(sync->currentPlayer, Cell::O);
    EXPECT_EQ(sync->board.size(), 9u);
    EXPECT_EQ(sync->board[4], Cell::X);         // клетка (1,1)
    EXPECT_FALSE(sync->gameOver);
    EXPECT_TRUE(c1.closed);                     // старое соединение закрыто хостом
    EXPECT_EQ(host.state(), NetworkHost::State::Playing);

    // Игра идёт уже через новое соединение
    c2.send(Move{{0, 0}});
    pump({&first, &second});
    EXPECT_FALSE(host.board().isEmpty(0, 0));
    EXPECT_EQ(c2.count<MoveApplied>(), 1);
}

TEST(NetworkHostTest, PingGetsPong) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    TestClient client(link.b());
    connect(host, link, client);

    client.send(Ping{});
    pump({&link});
    EXPECT_EQ(client.count<Pong>(), 1);
}

TEST(NetworkHostTest, ByeAbortsTheGame) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    GuiRecorder gui;
    host.setObserver(&gui);
    TestClient client(link.b());
    connect(host, link, client);

    client.send(Bye{});
    pump({&link});

    EXPECT_EQ(gui.aborted, 1);
    EXPECT_EQ(host.state(), NetworkHost::State::Finished);
    EXPECT_FALSE(host.makeHumanMove({0, 0}));
    EXPECT_TRUE(client.closed);
}

TEST(NetworkHostTest, GarbageFromClientAbortsTheGame) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    GuiRecorder gui;
    host.setObserver(&gui);
    TestClient client(link.b());
    connect(host, link, client);

    client.sendRaw("this is not json");
    pump({&link});

    ASSERT_NE(client.last<Error>(), nullptr);
    EXPECT_EQ(client.last<Error>()->code, "bad_message");
    EXPECT_TRUE(client.closed);
    EXPECT_EQ(gui.aborted, 1);
}

TEST(NetworkHostTest, LostConnectionAbortsTheGameForNow) {
    // Часть Б заменит это поведение на паузу и ожидание возвращения клиента
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    GuiRecorder gui;
    host.setObserver(&gui);
    TestClient client(link.b());
    connect(host, link, client);

    link.cut();
    pump({&link});

    EXPECT_EQ(gui.aborted, 1);
    EXPECT_EQ(host.state(), NetworkHost::State::Finished);
}

TEST(NetworkHostTest, HostGuiSeesMovesFromBothSides) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    GuiRecorder gui;
    host.setObserver(&gui);
    TestClient client(link.b());
    connect(host, link, client);

    host.makeHumanMove({1, 1});
    client.send(Move{{0, 0}});
    pump({&link});
    EXPECT_EQ(gui.moves, 2);
}