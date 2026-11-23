#pragma once
#include "IConnection.h"
#include "LineSplitter.h"
#include <QByteArray>
#include <QString>
#include <functional>
#include <memory>
#include <string>

class QTcpSocket;

// IConnection поверх QTcpSocket (см. контракт в IConnection.h).
//
// Что делает сверх сокета:
//  - нарезает поток на строки (LineSplitter); строка длиннее proto::kMaxLineBytes
//    считается нарушением, и соединение закрывается как оборванное;
//  - до установки соединения складывает отправляемые строки в очередь;
//  - close() сначала дописывает неотправленное (disconnectFromHost), а потом закрывает,
//    и после своего close() onClosed не вызывается;
//  - соединение принято сервером (сокет уже подключён) или создано клиентом (connectToHost).
//
// Клиентская сторона: сначала передай соединение в NetworkClient::connect (он выставит
// обработчики и отправит hello, строка уйдёт, когда соединение установится), и только потом
// вызывай connectToHost.
//
// Удалять объект внутри его собственных обработчиков нельзя. Для серверных соединений это
// делает QtHostServer (отложенно). Клиентское соединение должно жить дольше NetworkClient.
class QtTcpConnection : public IConnection {
public:
    // socket == nullptr: создать свой сокет (клиентская сторона).
    // Иначе берёт во владение уже подключённый сокет (его вернул QTcpServer).
    explicit QtTcpConnection(QTcpSocket* socket = nullptr);
    ~QtTcpConnection() override;
    QtTcpConnection(const QtTcpConnection&) = delete;
    QtTcpConnection& operator=(const QtTcpConnection&) = delete;

    void connectToHost(const QString& host, quint16 port);

    // --- IConnection ---
    void send(const std::string& line) override;
    void close() override;
    bool isOpen() const override;
    void setOnLine(std::function<void(const std::string&)> callback) override;
    void setOnClosed(std::function<void()> callback) override;

    // Для владельца соединения: вызывается один раз, когда соединение завершилось
    // по любой причине (в том числе после нашего close()). Тут можно планировать удаление.
    void setOnFinished(std::function<void(QtTcpConnection*)> callback);

private:
    enum class State { Connecting, Open, Closing, Closed };

    void wire();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onError();
    void finish();

    QTcpSocket* socket_;
    State state_ = State::Connecting;
    LineSplitter splitter_;
    QByteArray outbox_;                 // что отправить, когда соединение установится
    std::shared_ptr<bool> alive_;       // защита отложенных вызовов после уничтожения объекта

    std::function<void(const std::string&)> onLine_;
    std::function<void()> onClosed_;
    std::function<void(QtTcpConnection*)> onFinished_;
};