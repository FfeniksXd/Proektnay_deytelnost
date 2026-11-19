#pragma once
#include "Board.h"
#include "GameRules.h"
#include "IPlayer.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

enum class PlayerType { Human, AI };

struct SessionConfig {
    GameMode mode;
    PlayerType playerX;
    PlayerType playerO;
    int aiDifficulty = 1; // 1=легко, 2=средне, 3=сложно
};

// Интерфейс для GUI (чтобы окно знало, когда обновляться)
class IGameObserver {
public:
    virtual ~IGameObserver() = default;
    virtual void onBoardChanged(const Board& board) = 0;
    virtual void onTurnChanged(Cell currentPlayer) = 0;
    virtual void onGameOver(std::optional<Cell> winner) = 0;
};

// Запись об одном сделанном ходе (номер хода = позиция в списке истории)
struct MoveRecord {
    Position pos;
    Cell player;
};

class GameSession {
public:
    explicit GameSession(SessionConfig config);

    void setObserver(IGameObserver* observer);
    void start();

    // GUI вызывает этот метод, когда пользователь кликает по клетке
    bool makeHumanMove(Position p);

    void undo();
    void restart();

    const Board& board() const;
    Cell currentPlayer() const;
    bool isGameOver() const;
    std::optional<Cell> winner() const;

private:
    GameRules rules_;
    Board board_;
    Cell currentPlayer_;
    bool isGameOver_;
    std::optional<Cell> winner_;

    IGameObserver* observer_;

    // Умные указатели на игроков (если это ИИ, указатель не пустой)
    std::unique_ptr<IPlayer> playerX_;
    std::unique_ptr<IPlayer> playerO_;

    // История ходов (нужна для undo)
    std::vector<MoveRecord> history_;

    // Состояние ожидания ответа от игрока (ИИ, сеть, модель)
    std::uint64_t requestId_ = 0; // номер текущего запроса хода
    bool waiting_ = false;        // запрос отправлен, ответа ещё нет
    bool advancing_ = false;      // сейчас работает цикл advance() (защита от рекурсии)
    bool stalled_ = false;        // игрок не смог дать валидный ход

    // Приватные вспомогательные методы
    IPlayer* currentAI() const;   // не-человек текущего игрока, или nullptr, если ходит человек
    bool applyMove(Position p);   // единственное место, где ход ставится на доску
    void advance();               // пока ходит не человек, запрашивать ходы
    void onMoveReady(std::uint64_t id, std::optional<Position> move); // ответ игрока
    void resetPending();          // сбросить ожидание (старт, откат): старые ответы устаревают
    void checkGameEnd();
    void notifyBoardChanged();
    void notifyTurnChanged();
};