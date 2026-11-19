#pragma once
#include "Board.h"
#include "GameRules.h"
#include "IGameController.h"
#include "IGameObserver.h"
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

// Запись об одном сделанном ходе (номер хода = позиция в списке истории)
struct MoveRecord {
    Position pos;
    Cell player;
};

// Локальная сессия: ведёт партию, следит за очередью, вызывает игроков, уведомляет GUI
class GameSession : public IGameController {
public:
    explicit GameSession(SessionConfig config);

    // --- IGameController ---
    void setObserver(IGameObserver* observer) override;
    void start() override;
    bool makeHumanMove(Position p) override;

    void requestUndo() override;
    void respondToUndo(bool accept) override;
    void requestRestart() override;
    void respondToRestart(bool accept) override;

    const Board& board() const override;
    Cell currentPlayer() const override;
    bool isGameOver() const override;
    std::optional<Cell> winner() const override;

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

    // История ходов (нужна для отката)
    std::vector<MoveRecord> history_;

    // Состояние ожидания ответа от игрока
    std::uint64_t requestId_ = 0;  // номер текущего запроса хода
    bool waiting_ = false;         // ждём ответ от игрока
    bool advancing_ = false;       // сейчас работает advance() (защита от рекурсии)
    bool stalled_ = false;         // игрок не смог дать валидный ход
    bool thinkingShown_ = false;   // GUI уже знает, что игрок думает
    Cell thinkingPlayer_ = Cell::X;

    // Приватные вспомогательные методы
    IPlayer* currentAI() const;    // ИИ текущего игрока, или nullptr, если ходит человек
    bool applyMove(Position p);    // единственное место, где ход ставится на доску
    void advance();                // пока ходит не человек, запрашиваем ходы
    void onMoveReady(std::uint64_t id, std::optional<Position> move);
    void resetPending();           // сбросить ожидание (откат, перезапуск)
    void setThinking(bool thinking);
    void checkGameEnd();
    void notifyBoardChanged();
    void notifyTurnChanged();
};