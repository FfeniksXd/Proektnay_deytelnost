#include "Codec.h"
#include <nlohmann/json.hpp>
#include <climits>
#include <stdexcept>

using nlohmann::json;

namespace proto {
namespace {

// ============ Кодирование ============

std::string playerToString(Cell c) {
    if (c == Cell::X) return "X";
    if (c == Cell::O) return "O";
    throw std::invalid_argument("player must be X or O");
}

json winnerToJson(const std::optional<Cell>& winner) {
    if (!winner) return json(nullptr);
    return json(playerToString(*winner));
}

const char* reasonToString(RejectReason r) {
    switch (r) {
        case RejectReason::NotYourTurn: return "not_your_turn";
        case RejectReason::InvalidCell: return "invalid_cell";
        case RejectReason::GameOver:    return "game_over";
    }
    throw std::invalid_argument("unknown reject reason");
}

json make(const char* type) {
    json j = json::object();
    j["type"] = type;
    return j;
}

json toJson(const Hello& m) {
    json j = make("hello");
    j["protocol"] = m.protocol;
    if (m.resumeToken) j["resumeToken"] = *m.resumeToken;
    return j;
}

json toJson(const Welcome& m) {
    json j = make("welcome");
    j["protocol"] = m.protocol;
    json mode = json::object();
    mode["name"] = m.mode.name;
    mode["boardSize"] = m.mode.boardSize;
    mode["winLength"] = m.mode.winLength;
    mode["misere"] = m.mode.misere;
    j["mode"] = mode;
    j["yourSymbol"] = playerToString(m.yourSymbol);
    j["resumeToken"] = m.resumeToken;
    return j;
}

json toJson(const Sync& m) {
    json j = make("sync");
    std::string board;
    board.reserve(m.board.size());
    for (Cell c : m.board) {
        board += (c == Cell::X) ? 'X' : (c == Cell::O) ? 'O' : '.';
    }
    j["board"] = board;
    j["currentPlayer"] = playerToString(m.currentPlayer);
    j["moveCount"] = m.moveCount;
    j["gameOver"] = m.gameOver;
    j["winner"] = winnerToJson(m.winner);
    return j;
}

json toJson(const Move& m) {
    json j = make("move");
    j["row"] = m.pos.row;
    j["col"] = m.pos.col;
    return j;
}

json toJson(const MoveApplied& m) {
    json j = make("move_applied");
    j["moveNo"] = m.moveNo;
    j["player"] = playerToString(m.player);
    j["row"] = m.pos.row;
    j["col"] = m.pos.col;
    return j;
}

json toJson(const MoveRejected& m) {
    json j = make("move_rejected");
    j["reason"] = reasonToString(m.reason);
    return j;
}

json toJson(const UndoRequest& m) {
    json j = make("undo_request");
    j["moveNo"] = m.moveNo;
    return j;
}

json toJson(const UndoResponse& m) {
    json j = make("undo_response");
    j["moveNo"] = m.moveNo;
    j["accept"] = m.accept;
    return j;
}

json toJson(const UndoApplied& m) {
    json j = make("undo_applied");
    j["movesLeft"] = m.movesLeft;
    return j;
}

json toJson(const UndoDeclined& m) {
    json j = make("undo_declined");
    j["moveNo"] = m.moveNo;
    return j;
}

json toJson(const RestartRequest&) { return make("restart_request"); }

json toJson(const RestartResponse& m) {
    json j = make("restart_response");
    j["accept"] = m.accept;
    return j;
}

json toJson(const RestartApplied& m) {
    json j = make("restart_applied");
    j["yourSymbol"] = playerToString(m.yourSymbol);
    return j;
}

json toJson(const RestartDeclined&) { return make("restart_declined"); }

json toJson(const GameOver& m) {
    json j = make("game_over");
    j["winner"] = winnerToJson(m.winner);
    return j;
}

json toJson(const Error& m) {
    json j = make("error");
    j["code"] = m.code;
    j["text"] = m.text;
    return j;
}

json toJson(const Ping&) { return make("ping"); }
json toJson(const Pong&) { return make("pong"); }
json toJson(const Bye&)  { return make("bye"); }

// ============ Разбор ============

struct DecodeError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

const json& field(const json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end()) {
        throw DecodeError(std::string("missing field '") + key + "'");
    }
    return *it;
}

int getInt(const json& j, const char* key) {
    const json& v = field(j, key);
    if (!v.is_number_integer()) {
        throw DecodeError(std::string("field '") + key + "' must be an integer");
    }
    const long long x = v.get<long long>();
    if (x < INT_MIN || x > INT_MAX) {
        throw DecodeError(std::string("field '") + key + "' is out of range");
    }
    return static_cast<int>(x);
}

bool getBool(const json& j, const char* key) {
    const json& v = field(j, key);
    if (!v.is_boolean()) {
        throw DecodeError(std::string("field '") + key + "' must be true or false");
    }
    return v.get<bool>();
}

std::string getString(const json& j, const char* key) {
    const json& v = field(j, key);
    if (!v.is_string()) {
        throw DecodeError(std::string("field '") + key + "' must be a string");
    }
    return v.get<std::string>();
}

Cell stringToPlayer(const std::string& s, const char* key) {
    if (s == "X") return Cell::X;
    if (s == "O") return Cell::O;
    throw DecodeError(std::string("field '") + key + "' must be \"X\" or \"O\"");
}

Cell getPlayer(const json& j, const char* key) {
    return stringToPlayer(getString(j, key), key);
}

// Поле обязательно, но может быть null (= нет победителя)
std::optional<Cell> getWinner(const json& j, const char* key) {
    const json& v = field(j, key);
    if (v.is_null()) return std::nullopt;
    if (!v.is_string()) {
        throw DecodeError(std::string("field '") + key + "' must be \"X\", \"O\" or null");
    }
    return stringToPlayer(v.get<std::string>(), key);
}

GameMode getMode(const json& j, const char* key) {
    const json& m = field(j, key);
    if (!m.is_object()) {
        throw DecodeError(std::string("field '") + key + "' must be an object");
    }
    GameMode mode{};
    mode.name = getString(m, "name");
    mode.boardSize = getInt(m, "boardSize");
    mode.winLength = getInt(m, "winLength");
    mode.misere = getBool(m, "misere");
    return mode;
}

std::vector<Cell> getBoard(const json& j, const char* key) {
    const std::string s = getString(j, key);
    std::vector<Cell> board;
    board.reserve(s.size());
    for (char ch : s) {
        if (ch == 'X') board.push_back(Cell::X);
        else if (ch == 'O') board.push_back(Cell::O);
        else if (ch == '.') board.push_back(Cell::Empty);
        else throw DecodeError(std::string("field '") + key + "' may contain only X, O and '.'");
    }
    return board;
}

RejectReason getReason(const json& j, const char* key) {
    const std::string s = getString(j, key);
    if (s == "not_your_turn") return RejectReason::NotYourTurn;
    if (s == "invalid_cell")  return RejectReason::InvalidCell;
    if (s == "game_over")     return RejectReason::GameOver;
    throw DecodeError(std::string("field '") + key + "' has unknown value '" + s + "'");
}

Message decodeMessage(const json& j) {
    if (!j.is_object()) {
        throw DecodeError("message must be a JSON object");
    }
    const std::string type = getString(j, "type");

    if (type == "hello") {
        Hello m;
        m.protocol = getInt(j, "protocol");
        auto it = j.find("resumeToken");
        if (it != j.end() && !it->is_null()) {
            if (!it->is_string()) throw DecodeError("field 'resumeToken' must be a string");
            m.resumeToken = it->get<std::string>();
        }
        return m;
    }
    if (type == "welcome") {
        Welcome m;
        m.protocol = getInt(j, "protocol");
        m.mode = getMode(j, "mode");
        m.yourSymbol = getPlayer(j, "yourSymbol");
        m.resumeToken = getString(j, "resumeToken");
        return m;
    }
    if (type == "sync") {
        Sync m;
        m.board = getBoard(j, "board");
        m.currentPlayer = getPlayer(j, "currentPlayer");
        m.moveCount = getInt(j, "moveCount");
        m.gameOver = getBool(j, "gameOver");
        m.winner = getWinner(j, "winner");
        return m;
    }
    if (type == "move") {
        Move m;
        m.pos.row = getInt(j, "row");
        m.pos.col = getInt(j, "col");
        return m;
    }
    if (type == "move_applied") {
        MoveApplied m;
        m.moveNo = getInt(j, "moveNo");
        m.player = getPlayer(j, "player");
        m.pos.row = getInt(j, "row");
        m.pos.col = getInt(j, "col");
        return m;
    }
    if (type == "move_rejected") {
        MoveRejected m;
        m.reason = getReason(j, "reason");
        return m;
    }
    if (type == "undo_request") {
        UndoRequest m;
        m.moveNo = getInt(j, "moveNo");
        return m;
    }
    if (type == "undo_response") {
        UndoResponse m;
        m.moveNo = getInt(j, "moveNo");
        m.accept = getBool(j, "accept");
        return m;
    }
    if (type == "undo_applied") {
        UndoApplied m;
        m.movesLeft = getInt(j, "movesLeft");
        return m;
    }
    if (type == "undo_declined") {
        UndoDeclined m;
        m.moveNo = getInt(j, "moveNo");
        return m;
    }
    if (type == "restart_request") {
        return RestartRequest{};
    }
    if (type == "restart_response") {
        RestartResponse m;
        m.accept = getBool(j, "accept");
        return m;
    }
    if (type == "restart_applied") {
        RestartApplied m;
        m.yourSymbol = getPlayer(j, "yourSymbol");
        return m;
    }
    if (type == "restart_declined") {
        return RestartDeclined{};
    }
    if (type == "game_over") {
        GameOver m;
        m.winner = getWinner(j, "winner");
        return m;
    }
    if (type == "error") {
        Error m;
        m.code = getString(j, "code");
        m.text = getString(j, "text");
        return m;
    }
    if (type == "ping") return Ping{};
    if (type == "pong") return Pong{};
    if (type == "bye")  return Bye{};

    throw DecodeError("unknown message type '" + type + "'");
}

} // namespace

std::string encode(const Message& message) {
    const json j = std::visit([](const auto& m) { return toJson(m); }, message);
    // compact (без пробелов и переводов строк); недопустимый UTF-8 заменяется, а не бросает
    return j.dump(-1, ' ', false, json::error_handler_t::replace);
}

DecodeResult decode(const std::string& line) {
    if (line.size() > kMaxLineBytes) {
        return {std::nullopt, "line is too long"};
    }
    try {
        const json j = json::parse(line);
        return {decodeMessage(j), std::string()};
    } catch (const DecodeError& e) {
        return {std::nullopt, e.what()};
    } catch (const json::exception& e) {
        return {std::nullopt, std::string("invalid JSON: ") + e.what()};
    }
}

} // namespace proto