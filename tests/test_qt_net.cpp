// Проверка адаптеров Qt на НАСТОЯЩИХ сокетах (127.0.0.1): 
#include <gtest/gtest.h>
#include "NetworkClient.h"
#include "NetworkHost.h"
#include "QtHostServer.h"
#include "QtScheduler.h"
#include "QtTcpConnection.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpSocket>
#include <QThread>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace {

const GameMode kClassic{"Classic", 3, 3, false};

// Крутим цикл событий Qt, пока не выполнится условие (или не истечёт время)
bool waitFor(const std::function<bool()>& condition, int timeoutMs = 3000) {
    QElapsedTimer timer;
    timer.start();
    while (!condition()) {
        if (timer.elapsed() > timeoutMs) return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(1);
    }
    return true;
}

void spin(int ms) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(1);
    }
}

struct Recorder {
    std::vector<std::string> lines;
    int closed = 0;
    void attach(IConnection& c) {
        c.setOnLine([this](const std::string& line) { lines.push_back(line); });
        c.setOnClosed([this] { ++closed; });
    }
};

// Сервер, который записывает всё, что приходит по принятым соединениям
struct ServerRig {
    Recorder rec;
    std::vector<IConnection*> accepted;
    QtHostServer server{[this](IConnection& c) {
        rec.attach(c);                 // обработчики ставим сразу, как это делает NetworkHost
        accepted.push_back(&c);
    }};
    bool listening = false;

    ServerRig() { listening = server.listen(0, QHostAddress::LocalHost); }
    quint16 port() const { return server.port(); }
};

bool connectRaw(QTcpSocket& raw, quint16 port) {
    raw.connectToHost(QHostAddress::LocalHost, port);
    return waitFor([&] { return raw.state() == QAbstractSocket::ConnectedState; });
}

struct Gui : IGameObserver {
    std::vector<std::string> errorCodes;
    int aborted = 0;
    void onBoardChanged(const Board&) override {}
    void onTurnChanged(Cell) override {}
    void onGameOver(std::optional<Cell>) override {}
    void onError(const std::string& code, const std::string&) override { errorCodes.push_back(code); }
    void onGameAborted(const std::string&) override { ++aborted; }
};

} // namespace

// ---------- Нарезка на строки на настоящем TCP ----------

TEST(QtNetTest, ServerAcceptsAndReceivesALine) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    QTcpSocket raw;
    ASSERT_TRUE(connectRaw(raw, rig.port()));

    raw.write("hello\n");
    ASSERT_TRUE(waitFor([&] { return rig.rec.lines.size() == 1; }));
    EXPECT_EQ(rig.rec.lines[0], "hello");
}

TEST(QtNetTest, TwoLinesInOneChunk) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    QTcpSocket raw;
    ASSERT_TRUE(connectRaw(raw, rig.port()));

    raw.write("one\ntwo\n");           // одним куском
    ASSERT_TRUE(waitFor([&] { return rig.rec.lines.size() == 2; }));
    EXPECT_EQ(rig.rec.lines[0], "one");
    EXPECT_EQ(rig.rec.lines[1], "two");
}

TEST(QtNetTest, OneLineInTwoParts) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    QTcpSocket raw;
    ASSERT_TRUE(connectRaw(raw, rig.port()));

    raw.write("par");
    raw.flush();
    spin(100);                          // первая половина дошла, строка не закончена
    EXPECT_TRUE(rig.rec.lines.empty());
    raw.write("tial\n");
    ASSERT_TRUE(waitFor([&] { return rig.rec.lines.size() == 1; }));
    EXPECT_EQ(rig.rec.lines[0], "partial");
}

TEST(QtNetTest, TooLongLineWithoutNewlineClosesConnection) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    QTcpSocket raw;
    ASSERT_TRUE(connectRaw(raw, rig.port()));

    raw.write(QByteArray(5000, 'x'));   // длиннее лимита и без '\n'
    ASSERT_TRUE(waitFor([&] { return rig.rec.closed == 1; }));
    EXPECT_TRUE(rig.rec.lines.empty());
    EXPECT_TRUE(waitFor([&] { return raw.state() == QAbstractSocket::UnconnectedState; }));
}

TEST(QtNetTest, TooLongCompletedLineClosesConnection) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    QTcpSocket raw;
    ASSERT_TRUE(connectRaw(raw, rig.port()));

    raw.write(QByteArray(5000, 'x') + "\n");
    ASSERT_TRUE(waitFor([&] { return rig.rec.closed == 1; }));
    EXPECT_TRUE(rig.rec.lines.empty());
}

// ---------- Закрытие ----------

TEST(QtNetTest, PeerCloseNotifiesExactlyOnce) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    QTcpSocket raw;
    ASSERT_TRUE(connectRaw(raw, rig.port()));
    ASSERT_TRUE(waitFor([&] { return !rig.accepted.empty(); }));

    raw.disconnectFromHost();
    ASSERT_TRUE(waitFor([&] { return rig.rec.closed == 1; }));
    spin(100);
    EXPECT_EQ(rig.rec.closed, 1);
}

TEST(QtNetTest, OurOwnCloseDoesNotNotify) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    QTcpSocket raw;
    ASSERT_TRUE(connectRaw(raw, rig.port()));
    ASSERT_TRUE(waitFor([&] { return !rig.accepted.empty(); }));

    rig.accepted[0]->close();
    EXPECT_TRUE(waitFor([&] { return raw.state() == QAbstractSocket::UnconnectedState; }));
    spin(100);
    EXPECT_EQ(rig.rec.closed, 0);       // контракт IConnection: после своего close() onClosed нет
}

TEST(QtNetTest, CloseDeliversPendingDataFirst) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    QTcpSocket raw;
    ASSERT_TRUE(connectRaw(raw, rig.port()));
    ASSERT_TRUE(waitFor([&] { return !rig.accepted.empty(); }));

    rig.accepted[0]->send("goodbye");   // как error перед закрытием отказанного клиента
    rig.accepted[0]->close();
    ASSERT_TRUE(waitFor([&] { return raw.state() == QAbstractSocket::UnconnectedState; }));
    EXPECT_EQ(raw.readAll(), QByteArray("goodbye\n"));
}

TEST(QtNetTest, SendAfterCloseIsIgnored) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    QTcpSocket raw;
    ASSERT_TRUE(connectRaw(raw, rig.port()));
    ASSERT_TRUE(waitFor([&] { return !rig.accepted.empty(); }));

    rig.accepted[0]->close();
    rig.accepted[0]->send("late");
    EXPECT_TRUE(waitFor([&] { return raw.state() == QAbstractSocket::UnconnectedState; }));
    EXPECT_TRUE(raw.readAll().isEmpty());
}

TEST(QtNetTest, ManyShortConnectionsDoNotCrash) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);
    for (int i = 0; i < 20; ++i) {
        QTcpSocket raw;
        ASSERT_TRUE(connectRaw(raw, rig.port()));
        raw.write("x\n");
        raw.flush();
        raw.disconnectFromHost();
        spin(5);
    }
    ASSERT_TRUE(waitFor([&] { return rig.rec.closed == 20; }, 5000));
    spin(50);                           // даём серверу удалить завершённые соединения
}

// ---------- Клиентская сторона ----------

TEST(QtNetTest, LinesSentBeforeConnectedAreQueued) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);

    QtTcpConnection client;
    client.send("early");               // соединения ещё нет: строка ждёт в очереди
    client.connectToHost("127.0.0.1", rig.port());

    ASSERT_TRUE(waitFor([&] { return rig.rec.lines.size() == 1; }));
    EXPECT_EQ(rig.rec.lines[0], "early");
}

TEST(QtNetTest, ClientReceivesLinesFromServer) {
    ServerRig rig;
    ASSERT_TRUE(rig.listening);

    QtTcpConnection client;
    Recorder clientRec;
    clientRec.attach(client);
    client.connectToHost("127.0.0.1", rig.port());
    ASSERT_TRUE(waitFor([&] { return !rig.accepted.empty(); }));

    rig.accepted[0]->send("a");
    rig.accepted[0]->send("b");
    ASSERT_TRUE(waitFor([&] { return clientRec.lines.size() == 2; }));
    EXPECT_EQ(clientRec.lines[0], "a");
    EXPECT_EQ(clientRec.lines[1], "b");
}

TEST(QtNetTest, ConnectionRefusedIsReportedOnce) {
    quint16 deadPort = 0;
    {
        ServerRig rig;                  // занимаем порт и тут же освобождаем: там никто не слушает
        ASSERT_TRUE(rig.listening);
        deadPort = rig.port();
    }
    QtTcpConnection client;
    Recorder rec;
    rec.attach(client);

    // На Windows отказ в подключении к закрытому порту сообщается не сразу (система повторяет
    // попытки), поэтому ждём долго и печатаем, сколько это заняло.
    QElapsedTimer refusedTimer;
    refusedTimer.start();
    client.connectToHost("127.0.0.1", deadPort);

    const bool reported = waitFor([&] { return rec.closed >= 1; }, 15000);
    std::cout << "connection refused reported after " << refusedTimer.elapsed() << " ms\n";
    ASSERT_TRUE(reported);
    spin(100);
    EXPECT_EQ(rec.closed, 1);
    EXPECT_FALSE(client.isOpen());
}

// ---------- Целая партия по настоящему TCP ----------

TEST(QtNetTest, FullGameBetweenHostAndClientOverRealTcp) {
    NetworkHost* hostPtr = nullptr;
    QtHostServer server([&](IConnection& c) { hostPtr->acceptConnection(c); });
    HostOptions options;
    options.side = HostSide::X;
    options.makeToken = [] { return std::string("T1"); };
    NetworkHost host(kClassic, options);
    hostPtr = &host;
    ASSERT_TRUE(server.listen(0, QHostAddress::LocalHost));

    QtTcpConnection connection;         // объявлено до клиента: должно пережить его
    NetworkClient client;
    client.connect(connection);         // сначала обработчики и hello...
    connection.connectToHost("127.0.0.1", server.port()); // ...потом само подключение

    ASSERT_TRUE(waitFor([&] { return client.state() == NetworkClient::State::Playing; }));
    EXPECT_EQ(client.mySymbol(), Cell::O);

    EXPECT_TRUE(host.makeHumanMove({1, 1}));
    ASSERT_TRUE(waitFor([&] { return !client.board().isEmpty(1, 1); }));

    EXPECT_TRUE(client.makeHumanMove({0, 0}));
    ASSERT_TRUE(waitFor([&] { return !host.board().isEmpty(0, 0); }));
    ASSERT_TRUE(waitFor([&] { return !client.board().isEmpty(0, 0); }));
    EXPECT_EQ(host.currentPlayer(), Cell::X);
}

TEST(QtNetTest, SecondRealClientIsRefusedWhileGameInProgress) {
    NetworkHost* hostPtr = nullptr;
    QtHostServer server([&](IConnection& c) { hostPtr->acceptConnection(c); });
    NetworkHost host(kClassic, HostOptions{HostSide::X, {}, {}});
    hostPtr = &host;
    ASSERT_TRUE(server.listen(0, QHostAddress::LocalHost));

    QtTcpConnection conn1;
    NetworkClient client1;
    client1.connect(conn1);
    conn1.connectToHost("127.0.0.1", server.port());
    ASSERT_TRUE(waitFor([&] { return client1.state() == NetworkClient::State::Playing; }));

    QtTcpConnection conn2;
    NetworkClient client2;
    Gui gui2;
    client2.setObserver(&gui2);
    client2.connect(conn2);
    conn2.connectToHost("127.0.0.1", server.port());

    ASSERT_TRUE(waitFor([&] { return client2.state() == NetworkClient::State::Finished; }));
    ASSERT_EQ(gui2.errorCodes.size(), 1u);     // причина дошла до окна до закрытия соединения
    EXPECT_EQ(gui2.errorCodes[0], "game_in_progress");
    EXPECT_EQ(client1.state(), NetworkClient::State::Playing);   // первая партия не пострадала
}

// ---------- Планировщик ----------

TEST(QtSchedulerTest, FiresOnceAfterDelay) {
    QtScheduler scheduler;
    int fired = 0;
    scheduler.scheduleAfter(30, [&] { ++fired; });
    EXPECT_EQ(fired, 0);
    ASSERT_TRUE(waitFor([&] { return fired == 1; }));
    spin(100);
    EXPECT_EQ(fired, 1);
}

TEST(QtSchedulerTest, CancelPreventsFiring) {
    QtScheduler scheduler;
    int fired = 0;
    const TimerId id = scheduler.scheduleAfter(30, [&] { ++fired; });
    scheduler.cancel(id);
    scheduler.cancel(id);               // повторная отмена безвредна
    spin(150);
    EXPECT_EQ(fired, 0);
}

TEST(QtSchedulerTest, TimersFireInOrderAndCanRescheduleThemselves) {
    QtScheduler scheduler;
    std::vector<int> order;
    scheduler.scheduleAfter(60, [&] { order.push_back(2); });
    scheduler.scheduleAfter(10, [&] {
        order.push_back(1);
        scheduler.scheduleAfter(10, [&] { order.push_back(3); }); // из обработчика
    });
    ASSERT_TRUE(waitFor([&] { return order.size() == 3; }));
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 3);
    EXPECT_EQ(order[2], 2);
}

TEST(QtSchedulerTest, ClockMovesForward) {
    QtScheduler scheduler;
    const std::int64_t before = scheduler.nowMs();
    spin(50);
    EXPECT_GE(scheduler.nowMs() - before, 40);
}

// Для QCoreApplication нужен свой main (поэтому линкуем GTest::gtest, а не gtest_main)
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}