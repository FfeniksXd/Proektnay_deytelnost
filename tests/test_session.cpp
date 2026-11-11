#include <gtest/gtest.h>
#include "GameSession.h"

TEST(SessionTest, AIPlaysAgainstAI) {
    SessionConfig config;
    config.mode = {"Classic", 3, 3, false};
    config.playerX = PlayerType::AI;
    config.playerO = PlayerType::AI;
    
    GameSession session(config);
    session.start(); // Здесь ИИ мгновенно сыграют всю партию

    // Считаем, сколько клеток заполнено
    int filledCells = 0;
    for (int r = 0; r < session.board().size(); ++r) {
        for (int c = 0; c < session.board().size(); ++c) {
            if (!session.board().isEmpty(r, c)) {
                filledCells++;
            }
        }
    }

    // На поле 3x3 должно быть ровно 9 ходов (доска заполнена)
    ASSERT_EQ(filledCells, 9);
}