#pragma once
#include "Codec.h"
#include "GameSession.h"
#include "IConnection.h"
#include "IGameController.h"
#include "IGameObserver.h"
#include "RemotePlayer.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

enum class HostSide { X, O, Random };

struct HostOptions {
    HostSide side = HostSide::Random;
    // Жребий для HostSide::Random: true = хост играет за X. Пусто = настоящий случайный.
    std::function<bool()> coinFlip;
    // Секретный токен для переподключения. Пусто = случайный.
    std::function<std::string()> makeToken;
};

// Сторона хоста сетевой игры. Для окна выглядит как IGameController (как и локальная
// GameSession), внутри владеет настоящей сессией и общается с клиентом по протоколу
// (docs/PROTOCOL.md).
//
// Состояния: WaitingForClient -> Playing -> Finished.
// Сессия создаётся только после рукопожатия: сторона (X/O) определяется при подключении,
// и до него нет удалённого игрока.
//
// Соединениями хост НЕ владеет: их создаёт и хранит слушающая сторона (адаптер Qt).
// Соединение должно жить, пока не сработал onClosed или пока хост не вызвал close().
//
// Реализована часть А: рукопожатие, ходы, рассылка событий, отказ посторонним,
// переподключение по токену. Отмена и перезапуск, ping и пауза на потерю связи
// будут в части Б (пока потеря связи завершает партию).
class NetworkHost : public IGameController, private IGameObserver {
public:
    enum class State { WaitingForClient, Playing, Finished };

    explicit NetworkHost(GameMode mode, HostOptions options = {});
    ~NetworkHost() override;
    NetworkHost(const NetworkHost&) = delete;
    NetworkHost& operator=(const NetworkHost&) = delete;

    // Новое входящее соединение (от слушающего сокета)
    void acceptConnection(IConnection& connection);

    State state() const;

    // --- IGameController (для окна хоста) ---
    void setObserver(IGameObserver* observer) override;
    void start() override;                       // партия начнётся сама, когда подключится клиент
    bool makeHumanMove(Position p) override;
    void requestUndo() override;                 // часть Б
    void respondToUndo(bool accept) override;    // часть Б
    void requestRestart() override;              // часть Б
    void respondToRestart(bool accept) override; // часть Б
    const Board& board() const override;
    Cell currentPlayer() const override;
    bool isGameOver() const override;
    std::optional<Cell> winner() const override;

private:
    // --- IGameObserver (события настоящей сессии) ---
    void onBoardChanged(const Board& board) override;
    void onTurnChanged(Cell currentPlayer) override;
    void onGameOver(std::optional<Cell> winner) override;
    void onMoveApplied(const MoveRecord& move, int moveNo) override;
    void onThinkingChanged(Cell player, bool thinking) override;
    void onPlayerStalled(Cell player) override;
    void onUndoRequested() override;
    void onUndoDeclined() override;
    void onRestartRequested() override;
    void onRestartDeclined() override;
    void onOpponentDisconnected(int secondsLeft) override;
    void onOpponentReconnected() override;
    void onGameAborted(const std::string& reason) override;
    void onError(const std::string& code, const std::string& text) override;

    // --- Работа с соединениями ---
    void onLine(IConnection* connection, const std::string& line);
    void onClosed(IConnection* connection);
    void onPendingLine(IConnection* connection, const std::string& line);
    void onActiveLine(const std::string& line);
    void onClientMove(Position pos);

    bool isPending(IConnection* connection) const;
    void removePending(IConnection* connection);
    void reject(IConnection* connection, const std::string& code, const std::string& text);
    void sendTo(IConnection* connection, const proto::Message& message);
    void sendToClient(const proto::Message& message);
    void dropActive();
    void abortGame(const std::string& reason);

    void startGameWith(IConnection* connection);
    void resumeWith(IConnection* connection);

    GameMode mode_;
    HostOptions options_;
    GameRules rules_;          // для проверки ходов клиента до передачи их сессии
    Board emptyBoard_;         // что показывать, пока сессии нет

    State state_ = State::WaitingForClient;
    IGameObserver* gui_ = nullptr;

    std::unique_ptr<GameSession> session_;
    RemotePlayer* remote_ = nullptr;   // принадлежит session_
    Cell remoteSymbol_ = Cell::O;
    std::string token_;
    int moveCount_ = 0;

    IConnection* active_ = nullptr;          // подтверждённый клиент
    std::vector<IConnection*> pending_;      // подключились, но ещё не прислали hello
};