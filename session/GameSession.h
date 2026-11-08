#pragma once
#include "Board.h"
#include "GameRules.h"
#include "IPlayer.h"
#include <memory>
#include <optional>

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
    // Тут будет внутренняя реализация (поля и приватные методы)
    
};