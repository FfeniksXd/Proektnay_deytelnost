#include "LineSplitter.h"

LineSplitter::LineSplitter(std::size_t maxLineBytes) : maxLine_(maxLineBytes) {}

void LineSplitter::append(const char* data, std::size_t size) {
    if (overflow_) return;
    buffer_.append(data, size);
}

bool LineSplitter::next(std::string& line) {
    if (overflow_) return false;
    for (;;) {
        const std::size_t pos = buffer_.find('\n');
        if (pos == std::string::npos) {
            if (buffer_.size() > maxLine_) overflow_ = true; // конца строки нет, а она уже слишком длинная
            return false;
        }
        if (pos > maxLine_) {
            overflow_ = true;
            return false;
        }
        line.assign(buffer_, 0, pos);
        buffer_.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue; // пустые строки не сообщения
        return true;
    }
}

bool LineSplitter::overflowed() const { return overflow_; }

void LineSplitter::clear() {
    buffer_.clear();
    overflow_ = false;
}