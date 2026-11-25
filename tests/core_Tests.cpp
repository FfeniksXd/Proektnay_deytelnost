#include <gtest/gtest.h>
#include "GameRules.h"

// ==========================================
// ГРУППА 1: Базовые проверки побед (Классика)
// ==========================================

TEST(GameRulesTest, HorizontalWin) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::X); board.set(0, 1, Cell::X); board.set(0, 2, Cell::X);
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::X);
}

TEST(GameRulesTest, VerticalWin) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::O); board.set(1, 0, Cell::O); board.set(2, 0, Cell::O);
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::O);
}

TEST(GameRulesTest, DiagonalWin) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::O); board.set(1, 1, Cell::O); board.set(2, 2, Cell::O);
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::O);
}

TEST(GameRulesTest, AntiDiagonalWin) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 2, Cell::X); board.set(1, 1, Cell::X); board.set(2, 0, Cell::X);
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::X);
}

// ==========================================
// ГРУППА 2: Режим Мизер (Кто построил - проиграл)
// ==========================================

TEST(GameRulesTest, MisereHorizontalWin) {
    GameMode mode{"Misere", 3, 3, true};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::X); board.set(0, 1, Cell::X); board.set(0, 2, Cell::X);
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::O); 
}

TEST(GameRulesTest, MisereVerticalWin) {
    GameMode mode{"Misere", 3, 3, true};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::X); board.set(1, 0, Cell::X); board.set(2, 0, Cell::X);
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::O);
}

TEST(GameRulesTest, MisereDiagonalWin) {
    GameMode mode{"Misere", 3, 3, true};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::O); board.set(1, 1, Cell::O); board.set(2, 2, Cell::O);
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::X);
}

// ==========================================
// ГРУППА 3: Ничьи
// ==========================================

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

TEST(GameRulesTest, MisereDraw) {
    GameMode mode{"Misere", 3, 3, true};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::X); board.set(0, 1, Cell::O); board.set(0, 2, Cell::X);
    board.set(1, 0, Cell::X); board.set(1, 1, Cell::O); board.set(1, 2, Cell::O);
    board.set(2, 0, Cell::O); board.set(2, 1, Cell::X); board.set(2, 2, Cell::X);
    EXPECT_FALSE(rules.winner(board).has_value());
    EXPECT_TRUE(rules.isDraw(board));
}

// ==========================================
// ГРУППА 4: Негативные тесты (Ничьей нет)
// ==========================================

TEST(GameRulesTest, EmptyBoardIsNotDraw) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3); 
    EXPECT_FALSE(rules.winner(board).has_value());
    EXPECT_FALSE(rules.isDraw(board));
}

TEST(GameRulesTest, InProgressIsNotDraw) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    board.set(1, 1, Cell::X); board.set(0, 0, Cell::O);
    EXPECT_FALSE(rules.winner(board).has_value());
    EXPECT_FALSE(rules.isDraw(board));
}

TEST(GameRulesTest, WonGameIsNotDraw) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::X); board.set(0, 1, Cell::X); board.set(0, 2, Cell::X);
    ASSERT_TRUE(rules.winner(board).has_value());
    EXPECT_FALSE(rules.isDraw(board));
}

// ==========================================
// ГРУППА 5: Ложные срабатывания и настройки
// ==========================================

TEST(GameRulesTest, NoWinWithOnlyTwoInARow) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::X); board.set(0, 1, Cell::X); board.set(2, 2, Cell::O);
    EXPECT_FALSE(rules.winner(board).has_value());
}

TEST(GameRulesTest, NoWinIfLineIsBroken) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    board.set(0, 0, Cell::X); board.set(0, 1, Cell::X); board.set(0, 2, Cell::O); board.set(1, 0, Cell::X);
    EXPECT_FALSE(rules.winner(board).has_value());
}

TEST(GameRulesTest, CustomWinLengthOnBigBoard) {
    GameMode mode{"Big", 4, 3, false};
    GameRules rules(mode);
    Board board(4);
    board.set(1, 0, Cell::X); board.set(1, 1, Cell::X); board.set(1, 2, Cell::X);
    auto result = rules.winner(board);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Cell::X);
}

// ==========================================
// ГРУППА 6: ЗАЩИТА ОТ ДУРАКА (Новые тесты)
// ==========================================

TEST(GameRulesTest, OutOfBoundsMove_ShouldNotCrash) {
    GameMode mode{"Classic", 3, 3, false};
    GameRules rules(mode);
    Board board(3);
    
    // Проверяем, что невалидные ходы отсекаются
    EXPECT_FALSE(rules.isValidMove(board, {-1, 0}));
    EXPECT_FALSE(rules.isValidMove(board, {0, 3}));
    EXPECT_FALSE(rules.isValidMove(board, {5, 5}));
}

TEST(GameRulesTest, WinLengthOne_ShouldBeFixedToThree) {
    // Пытаемся создать абсурдную игру (победа с 1 хода)
    GameMode mode{"Weird", 3, 1, false};
    GameRules rules(mode); // Конструктор должен исправить winLength на 3
    Board board(3);
    
    board.set(1, 1, Cell::X);
    
    // Если защита в конструкторе работает, победы с одного хода не будет
    EXPECT_FALSE(rules.winner(board).has_value());
}