#include "QtTcpConnection.h"
#include "Messages.h"
#include <QTcpSocket>
#include <QTimer>
#include <utility>

QtTcpConnection::QtTcpConnection(QTcpSocket* socket)
    : socket_(socket ? socket : new QTcpSocket()),
      splitter_(proto::kMaxLineBytes),
      alive_(std::make_shared<bool>(true)) {
    socket_->setParent(nullptr); // не отдаём владение серверу, сокет теперь наш
    state_ = (socket_->state() == QAbstractSocket::ConnectedState) ? State::Open : State::Connecting;
    wire();

    // Данные могли прийти раньше, чем мы подписались на readyRead
    if (state_ == State::Open && socket_->bytesAvailable() > 0) {
        QTimer::singleShot(0, socket_, [this, alive = alive_] {
            if (*alive) onReadyRead();
        });
    }
}

QtTcpConnection::~QtTcpConnection() {
    *alive_ = false;
    QObject::disconnect(socket_, nullptr, nullptr, nullptr); // сокет может прожить чуть дольше нас
    socket_->abort();
    socket_->deleteLater();
}

void QtTcpConnection::wire() {
    socket_->setSocketOption(QAbstractSocket::LowDelayOption, 1); // маленькие сообщения без задержки
    QObject::connect(socket_, &QAbstractSocket::connected, socket_, [this] { onConnected(); });
    QObject::connect(socket_, &QIODevice::readyRead, socket_, [this] { onReadyRead(); });
    QObject::connect(socket_, &QAbstractSocket::disconnected, socket_, [this] { onDisconnected(); });
    QObject::connect(socket_, &QAbstractSocket::errorOccurred, socket_,
                     [this](QAbstractSocket::SocketError) { onError(); });
}

void QtTcpConnection::connectToHost(const QString& host, quint16 port) {
    if (state_ != State::Connecting) return;
    socket_->connectToHost(host, port);
}

// ======================= IConnection =======================

void QtTcpConnection::send(const std::string& line) {
    if (state_ == State::Closing || state_ == State::Closed) return;
    QByteArray data(line.data(), static_cast<qsizetype>(line.size()));
    data.append('\n');
    if (state_ == State::Connecting) {
        outbox_.append(data);
    } else {
        socket_->write(data);
    }
}

void QtTcpConnection::close() {
    if (state_ == State::Closing || state_ == State::Closed) return;
    const bool wasConnecting = (state_ == State::Connecting);
    state_ = State::Closing;                  // с этого момента onClosed не вызываем
    if (wasConnecting) {
        socket_->abort();
        finish();
        return;
    }
    socket_->disconnectFromHost();            // сначала допишет отправляемое
    if (socket_->state() == QAbstractSocket::UnconnectedState) finish();
    // иначе finish() вызовется из сигнала disconnected
}

bool QtTcpConnection::isOpen() const {
    return state_ == State::Open || state_ == State::Connecting;
}

void QtTcpConnection::setOnLine(std::function<void(const std::string&)> callback) {
    onLine_ = std::move(callback);
}

void QtTcpConnection::setOnClosed(std::function<void()> callback) {
    onClosed_ = std::move(callback);
}

void QtTcpConnection::setOnFinished(std::function<void(QtTcpConnection*)> callback) {
    onFinished_ = std::move(callback);
}

// ======================= События сокета =======================

void QtTcpConnection::onConnected() {
    if (state_ != State::Connecting) return;
    state_ = State::Open;
    if (!outbox_.isEmpty()) {
        socket_->write(outbox_);
        outbox_.clear();
    }
}

void QtTcpConnection::onReadyRead() {
    const QByteArray data = socket_->readAll();
    if (state_ != State::Open) return;        // закрываемся: пришедшее уже не нужно

    splitter_.append(data.constData(), static_cast<std::size_t>(data.size()));
    std::string line;
    while (state_ == State::Open && splitter_.next(line)) {
        auto callback = onLine_;              // копия: обработчик может заменить сам себя
        if (callback) callback(line);
    }

    if (state_ == State::Open && splitter_.overflowed()) {
        // Нарушение протокола (слишком длинная строка): для владельца это обрыв соединения
        socket_->abort();
        finish();
    }
}

void QtTcpConnection::onDisconnected() {
    finish();
}

void QtTcpConnection::onError() {
    // При обрыве Qt следом присылает disconnected. При неудачном подключении (отказ, нет
    // хоста) его не будет, поэтому завершаемся сами.
    if (state_ == State::Connecting || socket_->state() == QAbstractSocket::UnconnectedState) {
        finish();
    }
}

void QtTcpConnection::finish() {
    if (state_ == State::Closed) return;
    const bool closedByUs = (state_ == State::Closing);
    state_ = State::Closed;
    outbox_.clear();

    if (!closedByUs) {
        auto callback = onClosed_;
        if (callback) callback();
    }
    auto finished = onFinished_;
    if (finished) finished(this);             // после этого к this не обращаемся
}