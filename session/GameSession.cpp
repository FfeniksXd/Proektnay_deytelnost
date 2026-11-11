#include "GameSession.h"
#include "AIPlayer.h"
#include <stdexcept>

GameSession::GameSession(SessionConfig config) 
    : rules_(config.mode), board_(config.mode.boardSize), 
      currentPlayer_(Cell::X), isGameOver_(false), winner_(std::nullopt), observer_(nullptr) {
    
    // Создаем игроков в зависимости от конфигурации
    if (config.playerX == PlayerType::AI) {
        playerX_ = std::make_unique<AIPlayer>();
    } else {
        playerX_ = nullptr; // Человек, за него ходит GUI
    }

    if (config.playerO == PlayerType::AI) {
        playerO_ = std::make_unique<AIPlayer>();
    } else {
        playerO_ = nullptr;
    }
}

void GameSession::setObserver(IGameObserver* observer) {
    observer_ = observer;
}

void GameSession::start() {
    board_.reset();
    currentPlayer_ = Cell::X;
    isGameOver_ = false;
    winner_ = std::nullopt;
    
    notifyBoardChanged();
    notifyTurnChanged();
    
    // Если первым ходит ИИ, он делает ход сразу
    makeAIMoveIfNecessary();
}

bool GameSession::makeHumanMove(Position p) {
    // Проверяем, не закончена ли игра и не ходит ли сейчас ИИ
    if (isGameOver_) return false;
    if (currentPlayer_ == Cell::X && playerX_) return false; // Ход ИИ
    if (currentPlayer_ == Cell::O && playerO_) return false; // Ход ИИ

    // Проверяем валидность хода через правила
    if (!rules_.isValidMove(board_, p)) return false;

    // Делаем ход
    board_.set(p.row, p.col, currentPlayer_);
    notifyBoardChanged();
    checkGameEnd();
    
    if (!isGameOver_) {
        switchTurn();
        makeAIMoveIfNecessary();
    }
    return true;
}

void GameSession::makeAIMoveIfNecessary() {
    if (isGameOver_) return;

    IPlayer* currentAi = nullptr;
    if (currentPlayer_ == Cell::X && playerX_) currentAi = playerX_.get();
    if (currentPlayer_ == Cell::O && playerO_) currentAi = playerO_.get();

    if (currentAi) {
        auto move = currentAi->chooseMove(board_, rules_, currentPlayer_);
        if (move) {
            board_.set(move->row, move->col, currentPlayer_);
            notifyBoardChanged();
            checkGameEnd();
            if (!isGameOver_) {
                switchTurn();
            }
        }
    }
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

void GameSession::switchTurn() {
    currentPlayer_ = (currentPlayer_ == Cell::X) ? Cell::O : Cell::X;
    notifyTurnChanged();
    // После смены хода, если это ИИ, он должен сходить (рекурсивно)
    makeAIMoveIfNecessary();
}

void GameSession::notifyBoardChanged() {
    if (observer_) observer_->onBoardChanged(board_);
}

void GameSession::notifyTurnChanged() {
    if (observer_) observer_->onTurnChanged(currentPlayer_);
}

void GameSession::undo() {
    // Пока оставим пустым, реализуем позже (нужно хранить историю ходов)
}

void GameSession::restart() {
    start();
}

const Board& GameSession::board() const { return board_; }
Cell GameSession::currentPlayer() const { return currentPlayer_; }
bool GameSession::isGameOver() const { return isGameOver_; }
std::optional<Cell> GameSession::winner() const { return winner_; }