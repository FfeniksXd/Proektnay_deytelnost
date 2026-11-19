#include "NetworkClient.h"
#include <utility>
#include <variant>

namespace {

constexpr int kMaxBoardSize = 60; // доска больше не поместилась бы в одну строку протокола

Cell otherPlayer(Cell c) { return c == Cell::X ? Cell::O : Cell::X; }

const char* rejectText(proto::RejectReason r) {
    switch (r) {
        case proto::RejectReason::NotYourTurn: return "Сейчас не ваш ход";
        case proto::RejectReason::InvalidCell: return "Эту клетку занять нельзя";
        case proto::RejectReason::GameOver:    return "Партия уже окончена";
    }
    return "Ход не принят";
}

} // namespace

NetworkClient::NetworkClient() : board_(3) {}

NetworkClient::~NetworkClient() {
    if (conn_) {
        conn_->setOnLine(nullptr);   // соединение живёт дольше клиента
        conn_->setOnClosed(nullptr);
        conn_->close();
    }
}

NetworkClient::State NetworkClient::state() const { return state_; }
Cell NetworkClient::mySymbol() const { return mySymbol_; }

// ======================= Подключение и выход =======================

void NetworkClient::connect(IConnection& connection) {
    if (state_ != State::Idle) return;
    conn_ = &connection;
    state_ = State::Connecting;
    conn_->setOnLine([this](const std::string& line) { onLine(line); });
    conn_->setOnClosed([this] { onClosed(); });
    send(proto::Hello{});
}

void NetworkClient::disconnect() {
    if (conn_ && state_ != State::Finished) {
        send(proto::Bye{});
    }
    closeConnection();
    pendingMove_ = false;
    state_ = State::Finished;
}

void NetworkClient::send(const proto::Message& message) {
    if (conn_) conn_->send(proto::encode(message));
}

void NetworkClient::closeConnection() {
    IConnection* c = conn_;
    conn_ = nullptr;
    if (c) c->close();
}

// Фатальная ошибка: сообщаем окну, закрываем соединение, партия прервана
void NetworkClient::fail(const std::string& code, const std::string& text) {
    if (gui_) gui_->onError(code, text);
    closeConnection();
    pendingMove_ = false;
    const bool wasPlaying = (state_ == State::Playing);
    state_ = State::Finished;
    if (wasPlaying && gui_) gui_->onGameAborted(text);
}

// Нефатальная: странное сообщение, сообщаем окну, но игру не прерываем
void NetworkClient::unexpected(const std::string& text) {
    if (gui_) gui_->onError("unexpected_message", text);
}

// ======================= Приём сообщений =======================

void NetworkClient::onLine(const std::string& line) {
    proto::DecodeResult r = proto::decode(line);
    if (!r.message) {
        fail("bad_message", "Хост прислал неразборчивое сообщение: " + r.error);
        return;
    }
    handle(*r.message);
}

void NetworkClient::onClosed() {
    conn_ = nullptr;
    pendingMove_ = false;
    if (state_ == State::Connecting) {
        state_ = State::Finished;
        // Если хост объяснил причину отказа (error), окно её уже получило
        if (!errorReceived_ && gui_) {
            gui_->onError("connection_failed", "Не удалось подключиться к хосту");
        }
    } else if (state_ == State::Playing) {
        state_ = State::Finished;
        if (gui_) gui_->onGameAborted("Соединение с хостом потеряно"); // часть Б: пауза и возвращение
    }
}

void NetworkClient::handle(const proto::Message& m) {
    if (const auto* x = std::get_if<proto::Welcome>(&m)) {
        onWelcome(*x);
    } else if (const auto* x = std::get_if<proto::MoveApplied>(&m)) {
        onMoveApplied(*x);
    } else if (const auto* x = std::get_if<proto::MoveRejected>(&m)) {
        onMoveRejected(*x);
    } else if (const auto* x = std::get_if<proto::GameOver>(&m)) {
        onGameOver(*x);
    } else if (const auto* x = std::get_if<proto::Sync>(&m)) {
        onSync(*x);
    } else if (const auto* x = std::get_if<proto::Error>(&m)) {
        errorReceived_ = true;
        if (gui_) gui_->onError(x->code, x->text);
    } else if (std::holds_alternative<proto::Ping>(m)) {
        send(proto::Pong{});
    } else if (std::holds_alternative<proto::Pong>(m)) {
        // проверка живости (часть Б)
    } else {
        unexpected("Хост прислал сообщение, которое клиент пока не обрабатывает");
    }
}

void NetworkClient::onWelcome(const proto::Welcome& m) {
    if (state_ != State::Connecting) {
        unexpected("Повторное приветствие от хоста");
        return;
    }
    if (m.protocol != proto::kProtocolVersion) {
        fail("version_mismatch", "Версия протокола хоста не совпадает с версией клиента");
        return;
    }
    if (m.mode.boardSize < 1 || m.mode.boardSize > kMaxBoardSize) {
        fail("bad_welcome", "Хост прислал недопустимый размер доски");
        return;
    }

    mode_ = m.mode;
    board_ = Board(m.mode.boardSize);
    mySymbol_ = m.yourSymbol;
    token_ = m.resumeToken;
    currentPlayer_ = Cell::X;
    gameOver_ = false;
    winner_ = std::nullopt;
    moveCount_ = 0;
    pendingMove_ = false;
    state_ = State::Playing;

    if (gui_) {
        gui_->onGameStarted(mySymbol_);
        gui_->onBoardChanged(board_);
        gui_->onTurnChanged(currentPlayer_);
    }
}

void NetworkClient::onMoveApplied(const proto::MoveApplied& m) {
    if (state_ != State::Playing || gameOver_) {
        unexpected("Ход пришёл вне партии");
        return;
    }
    const bool inRange = m.pos.row >= 0 && m.pos.row < board_.size() &&
                         m.pos.col >= 0 && m.pos.col < board_.size();
    // Любое расхождение с нашей копией состояния значит, что мы потеряли синхронизацию
    if (m.moveNo != moveCount_ + 1 || m.player != currentPlayer_ || !inRange ||
        !board_.isEmpty(m.pos.row, m.pos.col)) {
        fail("out_of_sync", "Состояние партии у хоста и клиента разошлось");
        return;
    }

    board_.set(m.pos.row, m.pos.col, m.player);
    moveCount_ = m.moveNo;
    if (m.player == mySymbol_) pendingMove_ = false;

    if (gui_) {
        gui_->onBoardChanged(board_);
        gui_->onMoveApplied(MoveRecord{m.pos, m.player}, m.moveNo);
    }
    currentPlayer_ = otherPlayer(m.player);
    if (gui_) gui_->onTurnChanged(currentPlayer_);
}

void NetworkClient::onMoveRejected(const proto::MoveRejected& m) {
    pendingMove_ = false;
    if (gui_) gui_->onError("move_rejected", rejectText(m.reason));
}

void NetworkClient::onGameOver(const proto::GameOver& m) {
    if (state_ != State::Playing) {
        unexpected("Сообщение о конце партии пришло вне партии");
        return;
    }
    gameOver_ = true;
    winner_ = m.winner;
    pendingMove_ = false;
    state_ = State::Finished;
    if (gui_) gui_->onGameOver(winner_);
}

void NetworkClient::onSync(const proto::Sync& m) {
    if (state_ != State::Playing) {
        unexpected("Состояние партии пришло вне партии");
        return;
    }
    const int n = board_.size();
    if (static_cast<int>(m.board.size()) != n * n) {
        fail("bad_sync", "Состояние партии от хоста не подходит к размеру доски");
        return;
    }
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            board_.set(r, c, m.board[static_cast<std::size_t>(r * n + c)]);
        }
    }
    moveCount_ = m.moveCount;
    currentPlayer_ = m.currentPlayer;
    gameOver_ = m.gameOver;
    winner_ = m.winner;
    pendingMove_ = false;

    if (gui_) {
        gui_->onBoardChanged(board_);
        if (gameOver_) {
            state_ = State::Finished;
            gui_->onGameOver(winner_);
        } else {
            gui_->onTurnChanged(currentPlayer_);
        }
    } else if (gameOver_) {
        state_ = State::Finished;
    }
}

// ======================= IGameController =======================

void NetworkClient::setObserver(IGameObserver* observer) { gui_ = observer; }

void NetworkClient::start() {
    // Партия начинается, когда хост присылает welcome (см. connect).
}

bool NetworkClient::makeHumanMove(Position p) {
    if (state_ != State::Playing || gameOver_ || pendingMove_) return false;
    if (currentPlayer_ != mySymbol_) return false;
    if (p.row < 0 || p.row >= board_.size() || p.col < 0 || p.col >= board_.size()) return false;
    if (!board_.isEmpty(p.row, p.col)) return false;

    pendingMove_ = true;
    send(proto::Move{p});
    return true;
}

void NetworkClient::requestUndo() {}
void NetworkClient::respondToUndo(bool) {}
void NetworkClient::requestRestart() {}
void NetworkClient::respondToRestart(bool) {}

const Board& NetworkClient::board() const { return board_; }
Cell NetworkClient::currentPlayer() const { return currentPlayer_; }
bool NetworkClient::isGameOver() const { return gameOver_; }
std::optional<Cell> NetworkClient::winner() const { return winner_; }