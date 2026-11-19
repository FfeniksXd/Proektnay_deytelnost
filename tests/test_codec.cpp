#include <gtest/gtest.h>
#include "Codec.h"
#include <string>
#include <vector>

using namespace proto;

namespace {

std::vector<Message> allMessages() {
    Welcome welcome;
    welcome.mode = GameMode{"Classic", 3, 3, false};
    welcome.yourSymbol = Cell::O;
    welcome.resumeToken = "9f3c1a7e";

    Sync sync;
    sync.board = {Cell::X, Cell::Empty, Cell::O, Cell::Empty, Cell::X,
                  Cell::Empty, Cell::Empty, Cell::Empty, Cell::Empty};
    sync.currentPlayer = Cell::O;
    sync.moveCount = 3;

    Hello resume;
    resume.resumeToken = "9f3c1a7e";

    return {
        Hello{}, resume, welcome, sync,
        Move{{1, 2}}, MoveApplied{1, Cell::X, {1, 2}}, MoveRejected{RejectReason::NotYourTurn},
        UndoRequest{5}, UndoResponse{5, true}, UndoApplied{4}, UndoDeclined{5},
        RestartRequest{}, RestartResponse{false}, RestartApplied{Cell::X}, RestartDeclined{},
        GameOver{Cell::X}, GameOver{std::nullopt}, Error{"version_mismatch", "text"},
        Ping{}, Pong{}, Bye{}
    };
}

} // namespace

TEST(CodecTest, EveryMessageSurvivesRoundTrip) {
    for (const Message& m : allMessages()) {
        const std::string line = encode(m);
        EXPECT_EQ(line.find('\n'), std::string::npos) << line;
        EXPECT_LE(line.size(), kMaxLineBytes) << line;

        DecodeResult r = decode(line);
        ASSERT_TRUE(r.message.has_value()) << line << " -> " << r.error;
        EXPECT_EQ(r.message->index(), m.index()) << line;
        EXPECT_EQ(encode(*r.message), line);
    }
}

TEST(CodecTest, FieldsSurvive) {
    Welcome w;
    w.mode = GameMode{"Misere 4x4", 4, 3, true};
    w.yourSymbol = Cell::O;
    w.resumeToken = "tok";
    auto r = decode(encode(w));
    ASSERT_TRUE(r.message.has_value()) << r.error;
    const Welcome* got = std::get_if<Welcome>(&*r.message);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->mode.name, "Misere 4x4");
    EXPECT_EQ(got->mode.boardSize, 4);
    EXPECT_EQ(got->mode.winLength, 3);
    EXPECT_TRUE(got->mode.misere);
    EXPECT_EQ(got->yourSymbol, Cell::O);
    EXPECT_EQ(got->resumeToken, "tok");
}

TEST(CodecTest, DrawHasNoWinner) {
    auto r = decode(encode(GameOver{std::nullopt}));
    ASSERT_TRUE(r.message.has_value());
    EXPECT_FALSE(std::get<GameOver>(*r.message).winner.has_value());
}

TEST(CodecTest, HelloWithoutTokenOmitsIt) {
    const std::string line = encode(Hello{});
    EXPECT_EQ(line.find("resumeToken"), std::string::npos);
    auto r = decode(line);
    ASSERT_TRUE(r.message.has_value());
    EXPECT_FALSE(std::get<Hello>(*r.message).resumeToken.has_value());
}

TEST(CodecTest, DecodesExampleFromProtocolDocument) {
    auto r = decode(R"({"type":"move","row":1,"col":2})");
    ASSERT_TRUE(r.message.has_value()) << r.error;
    const Move& m = std::get<Move>(*r.message);
    EXPECT_EQ(m.pos.row, 1);
    EXPECT_EQ(m.pos.col, 2);
}

TEST(CodecTest, IgnoresUnknownFields) {
    auto r = decode(R"({"type":"ping","extra":123})");
    ASSERT_TRUE(r.message.has_value()) << r.error;
    EXPECT_TRUE(std::holds_alternative<Ping>(*r.message));
}

TEST(CodecTest, RejectsBadInput) {
    const std::vector<std::string> bad = {
        "not json at all",
        "[1,2,3]",
        "{}",                                              // нет type
        R"({"type":5})",                                   // type не строка
        R"({"type":"fly_to_moon"})",                       // неизвестный type
        R"({"type":"move","row":1})",                      // нет поля col
        R"({"type":"move","row":"1","col":2})",            // поле не того типа
        R"({"type":"move","row":1.5,"col":2})",            // не целое
        R"({"type":"move","row":99999999999,"col":2})",    // вне диапазона int
        R"({"type":"move_applied","moveNo":1,"player":"Z","row":0,"col":0})", // плохой игрок
        R"({"type":"move_rejected","reason":"because"})",  // неизвестная причина
        R"({"type":"game_over"})",                         // winner обязателен (может быть null)
        R"({"type":"sync","board":"X?O","currentPlayer":"X","moveCount":1,"gameOver":false,"winner":null})",
    };
    for (const std::string& line : bad) {
        auto r = decode(line);
        EXPECT_FALSE(r.message.has_value()) << line;
        EXPECT_FALSE(r.error.empty()) << line;
    }
}

TEST(CodecTest, RejectsTooLongLine) {
    const std::string line = R"({"type":"error","code":"x","text":")" + std::string(kMaxLineBytes, 'a') + R"("})";
    auto r = decode(line);
    EXPECT_FALSE(r.message.has_value());
}