#include "NetworkHost.h"
#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>
#include <utility>
#include <variant>

namespace {

bool realCoinFlip() {
    static std::mt19937 gen{std::random_device{}()};
    return std::uniform_int_distribution<int>(0, 1)(gen) == 0;
}

// Не криптографический, для игры в частной сети достаточно
std::string randomToken() {
    static std::mt19937_64 gen{std::random_device{}()};
    std::ostringstream out;
    out << std::hex << std::setfill('0')
        << std::setw(16) << gen() << std::setw(16) << gen();
    return out.str();
}

} // namespace

NetworkHost::NetworkHost(GameMode mode, HostOptions options)
    : mode_(mode), options_(std::move(options)), rules_(mode), emptyBoard_(mode.boardSize) {}

NetworkHost::~NetworkHost() {
    // Соединения живут дольше хоста: отцепляем обработчики, чтобы они не вызвали мёртвый объект
    auto detach = [](IConnection* c) {
        c->setOnLine(nullptr);
        c->setOnClosed(nullptr);
    };
    if (active_) {
        detach(active_);
        active_->close();
    }
    for (IConnection* c : pending_) detach(c);
}

NetworkHost::State NetworkHost::state() const { return state_; }

// ======================= Соединения =======================

void NetworkHost::acceptConnection(IConnection& connection) {
    IConnection* c = &connection;
    pending_.push_back(c);
    c->setOnLine([this, c](const std::string& line) { onLine(c, line); });
    c->setOnClosed([this, c] { onClosed(c); });
}

bool NetworkHost::isPending(IConnection* connection) const {
    return std::find(pending_.begin(), pending_.end(), connection) != pending_.end();
}

void NetworkHost::removePending(IConnection* connection) {
    pending_.erase(std::remove(pending_.begin(), pending_.end(), connection), pending_.end());
}

void NetworkHost::sendTo(IConnection* connection, const proto::Message& message) {
    if (connection) connection->send(proto::encode(message));
}

void NetworkHost::sendToClient(const proto::Message& message) {
    sendTo(active_, message);
}

// Отказ: сообщаем причину и закрываем соединение
void NetworkHost::reject(IConnection* connection, const std::string& code, const std::string& text) {
    sendTo(connection, proto::Error{code, text});
    removePending(connection);
    connection->close();
}

void NetworkHost::dropActive() {
    IConnection* c = active_;
    active_ = nullptr;
    if (c) c->close();
}

void NetworkHost::abortGame(const std::string& reason) {
    if (state_ != State::Playing) return;
    state_ = State::Finished;
    if (gui_) gui_->onGameAborted(reason);
}

void NetworkHost::onLine(IConnection* connection, const std::string& line) {
    if (connection == active_) {
        onActiveLine(line);
    } else if (isPending(connection)) {
        onPendingLine(connection, line);
    }
    // иначе это уже отброшенное соединение: игнорируем
}

void NetworkHost::onClosed(IConnection* connection) {
    if (connection == active_) {
        active_ = nullptr;
        abortGame("Соединение с соперником потеряно"); // часть Б заменит на паузу
    } else {
        removePending(connection);
    }
}

// ----- Новое соединение: ждём hello -----

void NetworkHost::onPendingLine(IConnection* connection, const std::string& line) {
    proto::DecodeResult r = proto::decode(line);
    if (!r.message) {
        reject(connection, "bad_message", r.error);
        return;
    }
    const proto::Hello* hello = std::get_if<proto::Hello>(&*r.message);
    if (!hello) {
        reject(connection, "unexpected_message", "expected hello");
        return;
    }
    if (hello->protocol != proto::kProtocolVersion) {
        reject(connection, "version_mismatch",
               "supported protocol version: " + std::to_string(proto::kProtocolVersion));
        return;
    }

    switch (state_) {
        case State::Finished:
            reject(connection, "game_finished", "the game is already over");
            return;
        case State::WaitingForClient:
            if (hello->resumeToken) {
                reject(connection, "unknown_session", "no such game");
                return;
            }
            removePending(connection);
            startGameWith(connection);
            return;
        case State::Playing:
            // Партия идёт: посторонних не пускаем. Исключение: верный токен доказывает,
            // что это тот же игрок (например, хост ещё не заметил обрыв старого соединения).
            if (hello->resumeToken && *hello->resumeToken == token_) {
                removePending(connection);
                resumeWith(connection);
            } else {
                reject(connection, "game_in_progress", "another player is already connected");
            }
            return;
    }
}

void NetworkHost::startGameWith(IConnection* connection) {
    active_ = connection;

    bool hostIsX = true;
    switch (options_.side) {
        case HostSide::X: hostIsX = true; break;
        case HostSide::O: hostIsX = false; break;
        case HostSide::Random:
            hostIsX = options_.coinFlip ? options_.coinFlip() : realCoinFlip();
            break;
    }
    remoteSymbol_ = hostIsX ? Cell::O : Cell::X;
    token_ = options_.makeToken ? options_.makeToken() : randomToken();

    auto remote = std::make_unique<RemotePlayer>();
    remote_ = remote.get();
    if (hostIsX) {
        session_ = std::make_unique<GameSession>(mode_, nullptr, std::move(remote));
    } else {
        session_ = std::make_unique<GameSession>(mode_, std::move(remote), nullptr);
    }
    session_->setObserver(this);
    state_ = State::Playing;
    moveCount_ = 0;

    proto::Welcome welcome;
    welcome.mode = mode_;
    welcome.yourSymbol = remoteSymbol_;
    welcome.resumeToken = token_;
    sendToClient(welcome);

    if (gui_) gui_->onGameStarted(hostIsX ? Cell::X : Cell::O);
    session_->start(); // welcome уже отправлен: клиент получит события после него
}

void NetworkHost::resumeWith(IConnection* connection) {
    IConnection* old = active_;
    active_ = connection;       // события старого соединения теперь игнорируются
    if (old) old->close();

    proto::Welcome welcome;
    welcome.mode = mode_;
    welcome.yourSymbol = remoteSymbol_;
    welcome.resumeToken = token_;
    sendToClient(welcome);

    proto::Sync sync;
    sync.board = session_->board().cells();
    sync.currentPlayer = session_->currentPlayer();
    sync.moveCount = moveCount_;
    sync.gameOver = session_->isGameOver();
    sync.winner = session_->winner();
    sendToClient(sync);
}

// ----- Подтверждённый клиент -----

void NetworkHost::onActiveLine(const std::string& line) {
    proto::DecodeResult r = proto::decode(line);
    if (!r.message) {
        sendToClient(proto::Error{"bad_message", r.error});
        dropActive();
        abortGame("Соперник прислал неразборчивое сообщение");
        return;
    }
    const proto::Message& m = *r.message;

    if (const proto::Move* move = std::get_if<proto::Move>(&m)) {
        onClientMove(move->pos);
    } else if (std::holds_alternative<proto::Ping>(m)) {
        sendToClient(proto::Pong{});
    } else if (std::holds_alternative<proto::Pong>(m)) {
        // проверка живости (часть Б)
    } else if (std::holds_alternative<proto::Bye>(m)) {
        dropActive();
        abortGame("Соперник вышел из игры");
    } else if (std::holds_alternative<proto::UndoRequest>(m) ||
               std::holds_alternative<proto::UndoResponse>(m) ||
               std::holds_alternative<proto::RestartRequest>(m) ||
               std::holds_alternative<proto::RestartResponse>(m)) {
        sendToClient(proto::Error{"not_supported", "undo and restart are not supported yet"});
    } else {
        sendToClient(proto::Error{"unexpected_message", "this message is not expected from a client"});
    }
}

// Ход клиента. Допустимость проверяет хост ДО передачи сессии: недопустимый ход,
// отданный сессии, остановил бы партию.
void NetworkHost::onClientMove(Position pos) {
    if (state_ != State::Playing) {
        sendToClient(proto::MoveRejected{proto::RejectReason::GameOver});
    } else if (!remote_ || !remote_->isWaiting()) {
        sendToClient(proto::MoveRejected{proto::RejectReason::NotYourTurn});
    } else if (!rules_.isValidMove(session_->board(), pos)) {
        sendToClient(proto::MoveRejected{proto::RejectReason::InvalidCell});
    } else {
        remote_->submitMove(pos); // дальше сессия сама пришлёт onMoveApplied
    }
}

// ======================= IGameController =======================

void NetworkHost::setObserver(IGameObserver* observer) { gui_ = observer; }

void NetworkHost::start() {
    // Партия начинается, когда подключается клиент (см. acceptConnection).
}

bool NetworkHost::makeHumanMove(Position p) {
    return state_ == State::Playing && session_ && session_->makeHumanMove(p);
}

void NetworkHost::requestUndo() {}
void NetworkHost::respondToUndo(bool) {}
void NetworkHost::requestRestart() {}
void NetworkHost::respondToRestart(bool) {}

const Board& NetworkHost::board() const {
    return session_ ? session_->board() : emptyBoard_;
}

Cell NetworkHost::currentPlayer() const {
    return session_ ? session_->currentPlayer() : Cell::X;
}

bool NetworkHost::isGameOver() const {
    return session_ && session_->isGameOver();
}

std::optional<Cell> NetworkHost::winner() const {
    if (!session_) return std::nullopt;
    return session_->winner();
}

// ======================= События сессии =======================
// Сессия -> окно хоста, а часть событий ещё и клиенту по сети.

void NetworkHost::onBoardChanged(const Board& board) {
    if (gui_) gui_->onBoardChanged(board);
}

void NetworkHost::onTurnChanged(Cell currentPlayer) {
    if (gui_) gui_->onTurnChanged(currentPlayer);
}

void NetworkHost::onMoveApplied(const MoveRecord& move, int moveNo) {
    moveCount_ = moveNo;
    sendToClient(proto::MoveApplied{moveNo, move.player, move.pos});
    if (gui_) gui_->onMoveApplied(move, moveNo);
}

void NetworkHost::onGameOver(std::optional<Cell> winner) {
    state_ = State::Finished;
    sendToClient(proto::GameOver{winner});
    if (gui_) gui_->onGameOver(winner);
}

void NetworkHost::onThinkingChanged(Cell player, bool thinking) {
    if (gui_) gui_->onThinkingChanged(player, thinking);
}
void NetworkHost::onPlayerStalled(Cell player) { if (gui_) gui_->onPlayerStalled(player); }
void NetworkHost::onUndoRequested() { if (gui_) gui_->onUndoRequested(); }
void NetworkHost::onUndoDeclined() { if (gui_) gui_->onUndoDeclined(); }
void NetworkHost::onRestartRequested() { if (gui_) gui_->onRestartRequested(); }
void NetworkHost::onRestartDeclined() { if (gui_) gui_->onRestartDeclined(); }
void NetworkHost::onOpponentDisconnected(int secondsLeft) {
    if (gui_) gui_->onOpponentDisconnected(secondsLeft);
}
void NetworkHost::onOpponentReconnected() { if (gui_) gui_->onOpponentReconnected(); }
void NetworkHost::onGameAborted(const std::string& reason) { if (gui_) gui_->onGameAborted(reason); }
void NetworkHost::onError(const std::string& code, const std::string& text) {
    if (gui_) gui_->onError(code, text);
}