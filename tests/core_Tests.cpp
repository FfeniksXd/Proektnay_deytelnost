#include <gtest/gtest.h>
#include "GameRules.h"

// Тест 1: Обычная победа по горизонтали
TEST(GameRulesTest, HorizontalWin) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    
    board.set(0, 0, Cell::X);
    board.set(0, 1, Cell::X);
    board.set(0, 2, Cell::X);
    
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::X);
}

// Тест 2: Победа по диагонали
TEST(GameRulesTest, DiagonalWin) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    
    board.set(0, 0, Cell::O);
    board.set(1, 1, Cell::O);
    board.set(2, 2, Cell::O);
    
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::O);
}

// Тест 3: Режим MISERE (Кто построил линию - проиграл)
TEST(GameRulesTest, MisereLogic) {
    GameMode mode{"Misere", 3, 3, true}; // Включаем мизер
    GameRules rules(mode);
    Board board(3);
    
    // X строит линию
    board.set(0, 0, Cell::X);
    board.set(0, 1, Cell::X);
    board.set(0, 2, Cell::X);
    
    auto result = rules.winner(board);
    // В мизере должен победить O, так как X построил линию и проиграл
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::O); 
}

// Тест 4: Проверка winLength на большом поле (4x4, победа по 3)
TEST(GameRulesTest, CustomWinLength) {
    GameMode mode{"Big", 4, 3, false};
    GameRules rules(mode);
    Board board(4);
    
    // Ставим 3 в ряд на поле 4x4
    board.set(1, 0, Cell::X);
    board.set(1, 1, Cell::X);
    board.set(1, 2, Cell::X);
    
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::X);
}

// Тест 5: Ничья (доска полна, победителя нет)
TEST(GameRulesTest, DrawGame) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    
    board.set(0, 0, Cell::X); board.set(0, 1, Cell::O); board.set(0, 2, Cell::X);
    board.set(1, 0, Cell::X); board.set(1, 1, Cell::O); board.set(1, 2, Cell::O);
    board.set(2, 0, Cell::O); board.set(2, 1, Cell::X); board.set(2, 2, Cell::X);
    
    EXPECT_FALSE(rules.winner(board).has_value());
    EXPECT_TRUE(rules.isDraw(board));
}