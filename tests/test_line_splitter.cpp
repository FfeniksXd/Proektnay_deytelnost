#include <gtest/gtest.h>
#include "LineSplitter.h"
#include <string>
#include <vector>

namespace {

void feed(LineSplitter& s, const std::string& bytes) { s.append(bytes.data(), bytes.size()); }

std::vector<std::string> drain(LineSplitter& s) {
    std::vector<std::string> out;
    std::string line;
    while (s.next(line)) out.push_back(line);
    return out;
}

} // namespace

TEST(LineSplitterTest, SplitsAtNewline) {
    LineSplitter s(100);
    feed(s, "hello\n");
    EXPECT_EQ(drain(s), std::vector<std::string>({"hello"}));
}

TEST(LineSplitterTest, TwoLinesInOneChunk) {
    LineSplitter s(100);
    feed(s, "one\ntwo\n");
    EXPECT_EQ(drain(s), std::vector<std::string>({"one", "two"}));
}

TEST(LineSplitterTest, OneLineInTwoChunks) {
    LineSplitter s(100);
    feed(s, "par");
    EXPECT_TRUE(drain(s).empty());          // строка ещё не закончилась
    feed(s, "tial\n");
    EXPECT_EQ(drain(s), std::vector<std::string>({"partial"}));
}

TEST(LineSplitterTest, ByteByByte) {
    LineSplitter s(100);
    std::vector<std::string> got;
    for (char c : std::string("ab\ncd\n")) {
        feed(s, std::string(1, c));
        for (const std::string& l : drain(s)) got.push_back(l);
    }
    EXPECT_EQ(got, std::vector<std::string>({"ab", "cd"}));
}

TEST(LineSplitterTest, TailWaitsForItsNewline) {
    LineSplitter s(100);
    feed(s, "one\ntw");
    EXPECT_EQ(drain(s), std::vector<std::string>({"one"}));
    feed(s, "o\n");
    EXPECT_EQ(drain(s), std::vector<std::string>({"two"}));
}

TEST(LineSplitterTest, StripsCarriageReturn) {
    LineSplitter s(100);
    feed(s, "windows\r\n");
    EXPECT_EQ(drain(s), std::vector<std::string>({"windows"}));
}

TEST(LineSplitterTest, SkipsEmptyLines) {
    LineSplitter s(100);
    feed(s, "\n\na\n\r\nb\n");
    EXPECT_EQ(drain(s), std::vector<std::string>({"a", "b"}));
}

TEST(LineSplitterTest, LineOfExactlyMaxLengthIsFine) {
    LineSplitter s(4);
    feed(s, "abcd\n");
    EXPECT_EQ(drain(s), std::vector<std::string>({"abcd"}));
    EXPECT_FALSE(s.overflowed());
}

TEST(LineSplitterTest, TooLongCompletedLineOverflows) {
    LineSplitter s(4);
    feed(s, "abcde\n");
    EXPECT_TRUE(drain(s).empty());
    EXPECT_TRUE(s.overflowed());
}

TEST(LineSplitterTest, TooLongUnfinishedLineOverflows) {
    LineSplitter s(4);
    feed(s, "abcde");                       // конца строки нет, а она уже длиннее лимита
    EXPECT_TRUE(drain(s).empty());
    EXPECT_TRUE(s.overflowed());
}

TEST(LineSplitterTest, ShortUnfinishedLineIsNotAnOverflow) {
    LineSplitter s(4);
    feed(s, "abcd");
    EXPECT_TRUE(drain(s).empty());
    EXPECT_FALSE(s.overflowed());
}

TEST(LineSplitterTest, GoodLinesBeforeOverflowAreStillDelivered) {
    LineSplitter s(4);
    feed(s, "ok\ntoolong\n");
    std::string line;
    ASSERT_TRUE(s.next(line));
    EXPECT_EQ(line, "ok");
    EXPECT_FALSE(s.next(line));
    EXPECT_TRUE(s.overflowed());
}

TEST(LineSplitterTest, OverflowIsSticky) {
    LineSplitter s(4);
    feed(s, "abcdef\n");
    drain(s);
    feed(s, "ok\n");                        // после нарушения строки больше не выдаются
    EXPECT_TRUE(drain(s).empty());
    s.clear();
    feed(s, "ok\n");
    EXPECT_EQ(drain(s), std::vector<std::string>({"ok"}));
    EXPECT_FALSE(s.overflowed());
}