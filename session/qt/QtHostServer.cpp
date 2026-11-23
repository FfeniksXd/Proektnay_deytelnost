#include "QtHostServer.h"
#include <QTimer>
#include <algorithm>
#include <utility>

QtHostServer::QtHostServer(std::function<void(IConnection&)> onConnection)
    : onConnection_(std::move(onConnection)) {
    QObject::connect(&server_, &QTcpServer::newConnection, &server_, [this] { onNewConnection(); });
}

QtHostServer::~QtHostServer() {
    server_.close();
    connections_.clear();
    graveyard_.clear();
}

bool QtHostServer::listen(quint16 port, const QHostAddress& address) {
    return server_.listen(address, port);
}

quint16 QtHostServer::port() const { return server_.serverPort(); }

QString QtHostServer::errorString() const { return server_.errorString(); }

void QtHostServer::close() { server_.close(); }

void QtHostServer::onNewConnection() {
    while (QTcpSocket* socket = server_.nextPendingConnection()) {
        auto connection = std::make_unique<QtTcpConnection>(socket);
        connection->setOnFinished([this](QtTcpConnection* c) { retire(c); });
        QtTcpConnection* raw = connection.get();
        connections_.push_back(std::move(connection));
        if (onConnection_) onConnection_(*raw);   // хост выставит свои обработчики
    }
}

// Соединение завершилось. Мы, скорее всего, находимся внутри его обработчика,
// поэтому не удаляем сразу, а откладываем на следующий виток цикла событий.
void QtHostServer::retire(QtTcpConnection* connection) {
    auto it = std::find_if(connections_.begin(), connections_.end(),
                           [connection](const std::unique_ptr<QtTcpConnection>& p) {
                               return p.get() == connection;
                           });
    if (it == connections_.end()) return;
    graveyard_.push_back(std::move(*it));
    connections_.erase(it);
    QTimer::singleShot(0, &context_, [this] { graveyard_.clear(); });
}