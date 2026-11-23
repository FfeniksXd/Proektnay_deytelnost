#pragma once
#include "IConnection.h"
#include "QtTcpConnection.h"
#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <functional>
#include <memory>
#include <vector>

// Слушающий сокет для хоста. Принимает входящие соединения, владеет ими и передаёт каждое
// в onConnection (обычно: [&](IConnection& c) { host.acceptConnection(c); }).
// Завершившиеся соединения удаляет отложенно, после того как отработают все их обработчики
// (удалять объект внутри его собственного обработчика нельзя).
//
// Порядок уничтожения: сервер должен жить дольше NetworkHost, потому что хост при
// уничтожении закрывает своё соединение. Объявляй сервер ПЕРЕД хостом.
class QtHostServer {
public:
    explicit QtHostServer(std::function<void(IConnection&)> onConnection);
    ~QtHostServer();
    QtHostServer(const QtHostServer&) = delete;
    QtHostServer& operator=(const QtHostServer&) = delete;

    // port == 0: система выберет свободный порт (узнать его можно через port())
    bool listen(quint16 port = 45454, const QHostAddress& address = QHostAddress::Any);
    quint16 port() const;
    QString errorString() const;
    void close();   // перестать принимать новые соединения; существующие продолжают жить

private:
    void onNewConnection();
    void retire(QtTcpConnection* connection);

    std::function<void(IConnection&)> onConnection_;
    QTcpServer server_;
    QObject context_;
    std::vector<std::unique_ptr<QtTcpConnection>> connections_;
    std::vector<std::unique_ptr<QtTcpConnection>> graveyard_;   // ждут удаления
};