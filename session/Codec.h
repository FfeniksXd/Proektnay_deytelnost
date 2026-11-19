#pragma once
#include "Messages.h"
#include <optional>
#include <string>

namespace proto {

// Сообщение -> одна строка JSON (без '\n' на конце и внутри).
std::string encode(const Message& message);

struct DecodeResult {
    std::optional<Message> message; // пусто, если строку разобрать не удалось
    std::string error;              // причина (для журнала и onError)
};

// Строка JSON -> сообщение. Никогда не бросает исключений: любая ошибка
// (не JSON, неизвестный type, нет поля, поле не того типа, слишком длинная строка)
// возвращается в error. Лишние неизвестные поля игнорируются.
DecodeResult decode(const std::string& line);

} // namespace proto