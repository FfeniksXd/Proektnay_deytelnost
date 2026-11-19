#include "GameSession.h"
#include "AIFactory.h"

namespace {
// Человек представлен пустым указателем (за него ходит GUI), ИИ создаёт фабрика
std::unique_ptr<IPlayer> makePlayer(PlayerType type, int difficulty) {
    if (type == PlayerType::AI) {
        return createAI(difficulty);
    }
    return nullptr;
}
} // namespace

GameSession::GameSession(SessionConfig config)
    : rules_(config.mode), board_(config.mode.boardSize),
      currentPlayer_(Cell::X), isGameOver_(false), winner_(std::nullopt), observer_(nullptr),
      playerX_(makePlayer(config.playerX, config.aiDifficulty)),
      playerO_(makePlayer(config.playerO, config.aiDifficulty)) {}

void GameSession::setObserver(IGameObserver* observer) {
    observer_ = observer;
}

void GameSession::start() {
    resetPending();
    board_.reset();
    history_.clear();
    currentPlayer_ = Cell::X;
    isGameOver_ = false;
    winner_ = std::nullopt;

    notifyBoardChanged();
    notifyTurnChanged();

    // Если первым ходит ИИ, он делает ход сразу
    advance();
}

// ИИ текущего игрока или nullptr, если сейчас ходит человек
IPlayer* GameSession::currentAI() const {
    return currentPlayer_ == Cell::X ? playerX_.get() : playerO_.get();
}

// Единственное место, где ход ставится на доску.
// Не знает, кто прислал ход (человек, ИИ, сеть) - ему это не важно.
bool GameSession::applyMove(Position p) {
    if (isGameOver_ || !rules_.isValidMove(board_, p)) return false;

    board_.set(p.row, p.col, currentPlayer_);
    history_.push_back({p, currentPlayer_}); // пишем ДО смены очереди
    notifyBoardChanged();

    checkGameEnd();
    // Если игра закончилась, очередь не меняем:
    // currentPlayer_ остаётся тем, кто сделал последний ход
    if (!isGameOver_) {
        currentPlayer_ = (currentPlayer_ == Cell::X) ? Cell::O : Cell::X;
        notifyTurnChanged();
    }
    return true;
}

// Пока ходит не человек, запрашиваем ходы.
// Игрок может ответить сразу (внутри requestMove) или позже; в обоих случаях
// стек не растёт: повторный вход в advance() запрещён флагом advancing_.
void GameSession::advance() {
    if (advancing_) return;
    advancing_ = true;

    while (!isGameOver_ && !waiting_ && !stalled_) {
        IPlayer* player = currentAI();
        if (!player) break; // очередь человека

        waiting_ = true;
        const std::uint64_t id = ++requestId_;
        player->requestMove(board_, rules_, currentPlayer_,
            [this, id](std::optional<Position> move) { onMoveReady(id, move); });
        // Ответил сразу: waiting_ уже false, цикл пойдёт дальше.
        // Ответит позже: цикл закончится, onMoveReady продолжит цепочку.
    }

    advancing_ = false;
}

void GameSession::onMoveReady(std::uint64_t id, std::optional<Position> move) {
    // Устаревший (позиция уже изменилась) или повторный ответ
    if (id != requestId_ || !waiting_) return;
    waiting_ = false;

    if (!move || !applyMove(*move)) {
        stalled_ = true; // игрок не справился: перестаём его спрашивать
        return;
    }
    advance();
}

// Сбросить ожидание: ответы на уже отправленные запросы станут устаревшими
void GameSession::resetPending() {
    ++requestId_;
    waiting_ = false;
    stalled_ = false;
    if (playerX_) playerX_->cancel();
    if (playerO_) playerO_->cancel();
}

bool GameSession::makeHumanMove(Position p) {
    // Игра закончена или сейчас ход ИИ
    if (isGameOver_ || currentAI()) return false;

    if (!applyMove(p)) return false;

    // После хода человека даём ответить ИИ
    advance();
    return true;
}

void GameSession::checkGameEnd() {
    auto win = rules_.winner(board_);
    if (win) {
        isGameOver_ = true;
        winner_ = win;
        if (observer_) observer_->onGameOver(winner_);
    } else if (rules_.isDraw(board_)) {
        isGameOver_ = true;
        winner_ = std::nullopt;
        if (observer_) observer_->onGameOver(winner_);
    }
}

void GameSession::notifyBoardChanged() {
    if (observer_) observer_->onBoardChanged(board_);
}

void GameSession::notifyTurnChanged() {
    if (observer_) observer_->onTurnChanged(currentPlayer_);
}

// Откат: убираем ходы, пока очередь не вернётся к человеку.
// Человек vs человек: откатывается 1 ход. Человек vs ИИ: пара ходов.
// Если ИИ ещё думает, откатывается только ход человека, ответ ИИ станет устаревшим.
void GameSession::undo() {
    if (history_.empty()) return;
    if (playerX_ && playerO_) return; // ИИ против ИИ: откат не нужен

    resetPending();

    do {
        MoveRecord last = history_.back();
        history_.pop_back();
        board_.set(last.pos.row, last.pos.col, Cell::Empty);
        currentPlayer_ = last.player; // очередь берём из записи, а не «переключаем»
    } while (!history_.empty() && currentAI());

    isGameOver_ = false;
    winner_ = std::nullopt;

    // Сначала сообщаем о результате отката...
    notifyBoardChanged();
    notifyTurnChanged();

    // ...и только потом даём ИИ ходить (нужно, если ИИ ходил первым)
    advance();
}

void GameSession::restart() {
    start();
}

const Board& GameSession::board() const { return board_; }
Cell GameSession::currentPlayer() const { return currentPlayer_; }
bool GameSession::isGameOver() const { return isGameOver_; }
std::optional<Cell> GameSession::winner() const { return winner_; }