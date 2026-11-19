#pragma once
#include <functional>
#include <string>

// Соединение с соперником. Умеет только одно: пересылать строки протокола.
// Что внутри (TCP-сокет Qt, тестовая заглушка в памяти) логика сессии не знает.
//
// Контракт:
//  - Одна строка = одно сообщение протокола, без завершающего '\n'
//    (границы сообщений - забота реализации).
//  - Строки доходят в том порядке, в каком отправлены, ничего не теряется,
//    пока соединение живо (свойство TCP).
//  - Обработчик onLine вызывается на каждую полученную строку.
//  - Обработчик onClosed вызывается ровно один раз, когда соединение закрыл
//    соперник или оно оборвалось. После нашего собственного close() он НЕ вызывается.
//  - send() на закрытом соединении молча игнорируется.
//  - Все обработчики вызываются в том потоке, где живёт сессия.
//    Обработчик вправе вызывать send() и close().
class IConnection {
public:
    virtual ~IConnection() = default;

    virtual void send(const std::string& line) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    virtual void setOnLine(std::function<void(const std::string&)> callback) = 0;
    virtual void setOnClosed(std::function<void()> callback) = 0;
};