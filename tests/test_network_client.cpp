#include <gtest/gtest.h>
#include "FakeConnection.h"
#include "NetworkClient.h"
#include "NetworkHost.h"
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

using namespace proto;

namespace {

const GameMode kClassic{"Classic", 3, 3, false};

// Окно: запоминает всё, что ему сообщили
struct Gui : IGameObserver {
    std::optional<Cell> started;
    std::vector<Cell> turns;
    std::vector<std::pair<MoveRecord, int>> moves;
    std::vector<std::optional<Cell>> gameOvers;
    std::vector<std::pair<std::string, std::string>> errors;
    int boardChanges = 0;
    int aborted = 0;

    void onBoardChanged(const Board&) override { ++boardChanges; }
    void onTurnChanged(Cell c) override { turns.push_back(c); }
    void onGameOver(std::optional<Cell> w) override { gameOvers.push_back(w); }
    void onGameStarted(Cell c) override { started = c; }
    void onMoveApplied(const MoveRecord& m, int n) override { moves.push_back({m, n}); }
    void onGameAborted(const std::string&) override { ++aborted; }
    void onError(const std::string& code, const std::string& text) override {
        errors.push_back({code, text});
    }
};

// Сторона «хоста» в тестах, где хост изображаем вручную
class Peer {
public:
    explicit Peer(IConnection& c) : conn_(c) {
        conn_.setOnLine([this](const std::string& line) {
            DecodeResult r = decode(line);
            if (r.message) inbox.push_back(*r.message);
        });
        conn_.setOnClosed([this] { closed = true; });
    }
    void send(const Message& m) { conn_.send(encode(m)); }
    template <class T> int count() const {
        int n = 0;
        for (const Message& m : inbox) if (std::holds_alternative<T>(m)) ++n;
        return n;
    }
    std::vector<Message> inbox;
    bool closed = false;

private:
    IConnection& conn_;
};

HostOptions hostOptions(HostSide side) {
    HostOptions o;
    o.side = side;
    o.makeToken = [] { return std::string("T1"); };
    return o;
}

void pump(std::initializer_list<FakeLink*> links) {
    for (int round = 0; round < 100; ++round) {
        bool any = false;
        for (FakeLink* l : links) {
            if (l->hasPending()) { l->deliverAll(); any = true; }
        }
        if (!any) return;
    }
}

Welcome welcome(Cell yourSymbol) {
    Welcome w;
    w.mode = kClassic;
    w.yourSymbol = yourSymbol;
    w.resumeToken = "T";
    return w;
}

// Подключить клиента к настоящему хосту
void join(NetworkHost& host, NetworkClient& client, FakeLink& link) {
    host.acceptConnection(link.a());
    client.connect(link.b());
    pump({&link});
}

} // namespace

// ---------- Клиент вместе с настоящим хостом ----------

TEST(NetworkClientTest, HandshakeStartsGameOnBothSides) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);

    join(host, client, link);

    EXPECT_EQ(client.state(), NetworkClient::State::Playing);
    EXPECT_EQ(host.state(), NetworkHost::State::Playing);
    EXPECT_EQ(client.mySymbol(), Cell::O);
    ASSERT_TRUE(gui.started.has_value());
    EXPECT_EQ(*gui.started, Cell::O);
    EXPECT_EQ(client.board().size(), 3);
    ASSERT_FALSE(gui.turns.empty());
    EXPECT_EQ(gui.turns.back(), Cell::X);
}

TEST(NetworkClientTest, MovesTravelBothWays) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);
    join(host, client, link);

    EXPECT_TRUE(host.makeHumanMove({1, 1}));
    pump({&link});
    EXPECT_FALSE(client.board().isEmpty(1, 1));
    EXPECT_EQ(client.currentPlayer(), Cell::O);

    EXPECT_TRUE(client.makeHumanMove({0, 0}));
    pump({&link});
    EXPECT_FALSE(host.board().isEmpty(0, 0));
    EXPECT_FALSE(client.board().isEmpty(0, 0));
    EXPECT_EQ(client.currentPlayer(), Cell::X);
    EXPECT_EQ(host.currentPlayer(), Cell::X);
    ASSERT_EQ(gui.moves.size(), 2u);
    EXPECT_EQ(gui.moves[0].second, 1);
    EXPECT_EQ(gui.moves[1].second, 2);
    EXPECT_EQ(gui.moves[1].first.player, Cell::O);
}

TEST(NetworkClientTest, SecondClickWhileWaitingIsIgnored) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    NetworkClient client;
    join(host, client, link);
    host.makeHumanMove({1, 1});
    pump({&link});

    EXPECT_TRUE(client.makeHumanMove({0, 0}));
    EXPECT_TRUE(client.board().isEmpty(0, 0));      // своего хода на доске пока нет
    EXPECT_FALSE(client.makeHumanMove({2, 2}));     // подтверждения ещё нет: второй клик не принят
    pump({&link});
    EXPECT_FALSE(client.board().isEmpty(0, 0));
    EXPECT_TRUE(client.board().isEmpty(2, 2));
}

TEST(NetworkClientTest, MoveOutOfTurnIsNotEvenSent) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));   // сейчас ход хоста
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);
    join(host, client, link);

    EXPECT_FALSE(client.makeHumanMove({0, 0}));
    pump({&link});
    EXPECT_TRUE(host.board().isEmpty(0, 0));
    EXPECT_TRUE(gui.errors.empty());                // хост ничего не отклонял: ход не уходил
}

TEST(NetworkClientTest, OccupiedOrOutOfRangeCellIsRefusedLocally) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    NetworkClient client;
    join(host, client, link);
    host.makeHumanMove({1, 1});
    pump({&link});

    EXPECT_FALSE(client.makeHumanMove({1, 1}));
    EXPECT_FALSE(client.makeHumanMove({-1, 0}));
    EXPECT_FALSE(client.makeHumanMove({0, 3}));
    EXPECT_TRUE(client.makeHumanMove({0, 0}));      // а нормальный проходит
}

TEST(NetworkClientTest, HostRefusalReachesTheWindow) {
    FakeLink first, second;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    NetworkClient c1, c2;
    Gui gui2;
    c2.setObserver(&gui2);
    join(host, c1, first);

    host.acceptConnection(second.a());
    c2.connect(second.b());
    pump({&first, &second});

    ASSERT_EQ(gui2.errors.size(), 1u);              // причина одна, без лишнего «не удалось подключиться»
    EXPECT_EQ(gui2.errors[0].first, "game_in_progress");
    EXPECT_EQ(c2.state(), NetworkClient::State::Finished);
    EXPECT_EQ(gui2.aborted, 0);                     // партия у него и не начиналась
    EXPECT_EQ(c1.state(), NetworkClient::State::Playing);
}

TEST(NetworkClientTest, DisconnectTellsHostBye) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    Gui hostGui;
    host.setObserver(&hostGui);
    NetworkClient client;
    join(host, client, link);

    client.disconnect();
    pump({&link});

    EXPECT_EQ(client.state(), NetworkClient::State::Finished);
    EXPECT_EQ(hostGui.aborted, 1);
}

TEST(NetworkClientTest, LostConnectionAbortsTheGameForNow) {
    FakeLink link;
    NetworkHost host(kClassic, hostOptions(HostSide::X));
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);
    join(host, client, link);

    link.cut();
    pump({&link});

    EXPECT_EQ(gui.aborted, 1);
    EXPECT_EQ(client.state(), NetworkClient::State::Finished);
    EXPECT_FALSE(client.makeHumanMove({0, 0}));
}

// ---------- Хост изображаем вручную ----------

TEST(NetworkClientTest, RejectedMoveClearsWaitingAndShowsError) {
    FakeLink link;
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);
    Peer peer(link.a());
    client.connect(link.b());
    peer.send(welcome(Cell::X));                    // клиент играет за X и ходит первым
    pump({&link});

    ASSERT_TRUE(client.makeHumanMove({0, 0}));
    pump({&link});
    EXPECT_EQ(peer.count<Move>(), 1);

    peer.send(MoveRejected{RejectReason::InvalidCell});
    pump({&link});
    ASSERT_EQ(gui.errors.size(), 1u);
    EXPECT_EQ(gui.errors[0].first, "move_rejected");
    EXPECT_TRUE(client.makeHumanMove({1, 1}));      // ожидание снято, можно ходить снова
}

TEST(NetworkClientTest, OutOfSyncMoveAbortsTheGame) {
    FakeLink link;
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);
    Peer peer(link.a());
    client.connect(link.b());
    peer.send(welcome(Cell::O));
    pump({&link});

    peer.send(MoveApplied{5, Cell::X, {0, 0}});     // ожидался ход номер 1
    pump({&link});

    ASSERT_EQ(gui.errors.size(), 1u);
    EXPECT_EQ(gui.errors[0].first, "out_of_sync");
    EXPECT_EQ(gui.aborted, 1);
    EXPECT_TRUE(peer.closed);
    EXPECT_TRUE(client.board().isEmpty(0, 0));
}

TEST(NetworkClientTest, SyncReplacesTheWholeState) {
    FakeLink link;
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);
    Peer peer(link.a());
    client.connect(link.b());
    peer.send(welcome(Cell::O));
    pump({&link});

    Sync sync;
    sync.board = {Cell::X, Cell::Empty, Cell::O, Cell::Empty, Cell::X,
                  Cell::Empty, Cell::Empty, Cell::Empty, Cell::Empty};
    sync.currentPlayer = Cell::O;
    sync.moveCount = 3;
    peer.send(sync);
    pump({&link});

    EXPECT_EQ(client.board().get(0, 0), Cell::X);
    EXPECT_EQ(client.board().get(0, 2), Cell::O);
    EXPECT_EQ(client.board().get(1, 1), Cell::X);
    EXPECT_EQ(client.currentPlayer(), Cell::O);
    EXPECT_EQ(gui.turns.back(), Cell::O);

    peer.send(MoveApplied{4, Cell::O, {2, 2}});     // нумерация продолжается с синхронизированного места
    pump({&link});
    EXPECT_EQ(client.board().get(2, 2), Cell::O);
    EXPECT_TRUE(gui.errors.empty());
}

TEST(NetworkClientTest, BadSyncSizeAbortsTheGame) {
    FakeLink link;
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);
    Peer peer(link.a());
    client.connect(link.b());
    peer.send(welcome(Cell::O));
    pump({&link});

    Sync sync;
    sync.board = {Cell::X, Cell::O};                // для 3x3 нужно 9 клеток
    peer.send(sync);
    pump({&link});

    ASSERT_EQ(gui.errors.size(), 1u);
    EXPECT_EQ(gui.errors[0].first, "bad_sync");
    EXPECT_EQ(gui.aborted, 1);
}

TEST(NetworkClientTest, GameOverReachesTheWindow) {
    FakeLink link;
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);
    Peer peer(link.a());
    client.connect(link.b());
    peer.send(welcome(Cell::O));
    pump({&link});

    peer.send(MoveApplied{1, Cell::X, {0, 0}});
    peer.send(GameOver{Cell::X});
    pump({&link});

    ASSERT_EQ(gui.gameOvers.size(), 1u);
    ASSERT_TRUE(gui.gameOvers[0].has_value());
    EXPECT_EQ(*gui.gameOvers[0], Cell::X);
    EXPECT_TRUE(client.isGameOver());
    ASSERT_TRUE(client.winner().has_value());
    EXPECT_EQ(*client.winner(), Cell::X);
    EXPECT_FALSE(client.makeHumanMove({1, 1}));
}

TEST(NetworkClientTest, PingGetsPong) {
    FakeLink link;
    NetworkClient client;
    Peer peer(link.a());
    client.connect(link.b());
    peer.send(welcome(Cell::O));
    peer.send(Ping{});
    pump({&link});
    EXPECT_EQ(peer.count<Pong>(), 1);
}

TEST(NetworkClientTest, WrongProtocolVersionInWelcomeIsFatal) {
    FakeLink link;
    NetworkClient client;
    Gui gui;
    client.setObserver(&gui);
    Peer peer(link.a());
    client.connect(link.b());

    Welcome w = welcome(Cell::O);
    w.protocol = 99;
    peer.send(w);
    pump({&link});

    ASSERT_EQ(gui.errors.size(), 1u);
    EXPECT_EQ(gui.errors[0].first, "version_mismatch");
    EXPECT_EQ(client.state(), NetworkClient::State::Finished);
    EXPECT_TRUE(peer.closed);
}