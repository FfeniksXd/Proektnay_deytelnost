#pragma once
#include <cstddef>
#include <string>

// Нарезка потока байтов на строки. TCP не сохраняет границы сообщений: две строки могут
// прийти одним куском, а одна строка двумя. Класс копит пришедшие байты и отдаёт готовые
// строки (оканчивающиеся на '\n'). Не зависит от Qt и сети, поэтому проверяется обычными тестами.
//
// Защита: строка длиннее maxLineBytes (готовая или ещё не завершённая) считается нарушением
// (overflowed()). Без неё чужая программа могла бы забить память «строкой» без конца.
class LineSplitter {
public:
    explicit LineSplitter(std::size_t maxLineBytes);

    // Добавить пришедшие байты
    void append(const char* data, std::size_t size);

    // Достать следующую готовую строку без '\n' (и без '\r' на конце).
    // Пустые строки пропускаются. false = готовых строк нет (или нарушен лимит).
    bool next(std::string& line);

    // true, если обнаружена строка длиннее лимита. После этого строки больше не выдаются.
    bool overflowed() const;

    void clear();

private:
    std::size_t maxLine_;
    std::string buffer_;
    bool overflow_ = false;
};