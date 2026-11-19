#pragma once
#include "Codec.h"
#include "IConnection.h"
#include "IGameController.h"
#include "IGameObserver.h"
#include <optional>
#include <string>

// Сторона клиента сетевой игры. Для окна выглядит как IGameController (как GameSession
// и NetworkHost), но настоящей партии не ведёт: правила применяет хост, а клиент
// хранит копию состояния, которую хост присылает (docs/PROTOCOL.md).
//
// Свой ход клиент НЕ ставит на доску сам: он отправляет его хосту и ждёт move_applied.
// Пока ответ не пришёл, повторные ходы не принимаются (makeHumanMove вернёт false).
//
// Состояния: Idle -> Connecting (отправили hello) -> Playing (пришёл welcome) -> Finished
// (партия окончена, прервана или соединение потеряно).
//
// Соединением клиент НЕ владеет (его создаёт адаптер Qt): оно должно жить, пока не сработал
// onClosed или пока клиент не вызвал close().
//
// Реализована часть А: подключение, ходы, конец игры, sync. Отмена и перезапуск, ping
// и переподключение будут в части Б (пока потеря связи завершает партию).
class NetworkClient : public IGameController {
public:
    enum class State { Idle, Connecting, Playing, Finished };

    NetworkClient();
    ~NetworkClient() override;
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Начать подключение (отправляет hello). Партия начнётся, когда придёт welcome.
    void connect(IConnection& connection);
    // Аккуратный выход (отправляет bye и закрывает соединение). Окну событий не шлёт.
    void disconnect();

    State state() const;
    Cell mySymbol() const;   // осмысленно после welcome

    // --- IGameController ---
    void setObserver(IGameObserver* observer) override;
    void start() override;                       // партия начнётся, когда хост пришлёт welcome
    bool makeHumanMove(Position p) override;     // true = ход отправлен хосту
    void requestUndo() override;                 // часть Б
    void respondToUndo(bool accept) override;    // часть Б
    void requestRestart() override;              // часть Б
    void respondToRestart(bool accept) override; // часть Б
    const Board& board() const override;
    Cell currentPlayer() const override;
    bool isGameOver() const override;
    std::optional<Cell> winner() const override;

private:
    void onLine(const std::string& line);
    void onClosed();
    void handle(const proto::Message& message);

    void onWelcome(const proto::Welcome& m);
    void onMoveApplied(const proto::MoveApplied& m);
    void onMoveRejected(const proto::MoveRejected& m);
    void onGameOver(const proto::GameOver& m);
    void onSync(const proto::Sync& m);

    void send(const proto::Message& message);
    void closeConnection();
    void fail(const std::string& code, const std::string& text);   // фатальная ошибка
    void unexpected(const std::string& text);                      // нефатальная

    IConnection* conn_ = nullptr;
    IGameObserver* gui_ = nullptr;

    State state_ = State::Idle;
    GameMode mode_{};
    Board board_;
    Cell mySymbol_ = Cell::X;
    Cell currentPlayer_ = Cell::X;
    bool gameOver_ = false;
    std::optional<Cell> winner_;
    int moveCount_ = 0;
    std::string token_;           // понадобится для переподключения (часть Б)
    bool pendingMove_ = false;    // свой ход отправлен, подтверждения ещё нет
    bool errorReceived_ = false;  // хост уже объяснил причину отказа сообщением error
};