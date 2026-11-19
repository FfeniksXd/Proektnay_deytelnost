#pragma once
// Сообщения сетевого протокола (см. docs/PROTOCOL.md): по одной структуре на каждое сообщение.
#include "Board.h"
#include "GameMode.h"
#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace proto {

constexpr int kProtocolVersion = 1;
constexpr std::size_t kMaxLineBytes = 4096; // максимальная длина строки с сообщением

enum class RejectReason { NotYourTurn, InvalidCell, GameOver };

// ---- Подключение ----

// Клиент -> хост. resumeToken указывается только при переподключении.
struct Hello {
    int protocol = kProtocolVersion;
    std::optional<std::string> resumeToken;
};

// Хост -> клиент: принять и настроить игру.
struct Welcome {
    int protocol = kProtocolVersion;
    GameMode mode{};
    Cell yourSymbol = Cell::X;
    std::string resumeToken;
};

// Хост -> клиент: полное состояние партии (после переподключения).
// board по строкам слева направо, сверху вниз. winner пустой = победителя нет.
struct Sync {
    std::vector<Cell> board;
    Cell currentPlayer = Cell::X;
    int moveCount = 0;
    bool gameOver = false;
    std::optional<Cell> winner;
};

// ---- Ходы ----

// Клиент -> хост: мой ход.
struct Move {
    Position pos{0, 0};
};

// Хост -> клиент: подтверждённый ход (чей угодно).
struct MoveApplied {
    int moveNo = 0;
    Cell player = Cell::X;
    Position pos{0, 0};
};

// Хост -> клиент: ход не принят.
struct MoveRejected {
    RejectReason reason = RejectReason::InvalidCell;
};

// ---- Отмена хода (в обе стороны) ----

struct UndoRequest { int moveNo = 0; };
struct UndoResponse { int moveNo = 0; bool accept = false; };
struct UndoApplied { int movesLeft = 0; };   // хост -> клиент
struct UndoDeclined { int moveNo = 0; };     // хост -> клиент

// ---- Перезапуск партии (в обе стороны) ----

struct RestartRequest {};
struct RestartResponse { bool accept = false; };
struct RestartApplied { Cell yourSymbol = Cell::X; }; // хост -> клиент
struct RestartDeclined {};                            // хост -> клиент

// ---- Прочее ----

// Хост -> клиент: партия окончена. winner пустой = ничья.
struct GameOver {
    std::optional<Cell> winner;
};

struct Error {
    std::string code;
    std::string text;
};

struct Ping {};
struct Pong {};
struct Bye {};

// Любое сообщение протокола
using Message = std::variant<
    Hello, Welcome, Sync,
    Move, MoveApplied, MoveRejected,
    UndoRequest, UndoResponse, UndoApplied, UndoDeclined,
    RestartRequest, RestartResponse, RestartApplied, RestartDeclined,
    GameOver, Error, Ping, Pong, Bye>;

} // namespace proto