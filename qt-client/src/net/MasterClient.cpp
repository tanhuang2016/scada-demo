/**
 * @file   MasterClient.cpp
 * @brief  Qt TCP 客户端实现：行缓冲 + UPDATE/OFFLINE/ONLINE 帧分发
 * @module qt-client
 */

#include "net/MasterClient.hpp"

#include <QTcpSocket>
#include <QByteArray>
#include <iostream>

#include "scada/protocol.hpp"
#include "scada/types.hpp"

MasterClient::MasterClient(QObject* parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , reconnectTimer_(new QTimer(this))
    , host_()
    , port_(0)
    , connected_(false)
{
    qRegisterMetaType<scada::Telemetry>("scada::Telemetry");

    QObject::connect(socket_, SIGNAL(connected()),
                     this, SLOT(onConnected()));
    QObject::connect(socket_, SIGNAL(disconnected()),
                     this, SLOT(onDisconnected()));
    QObject::connect(socket_, SIGNAL(readyRead()),
                     this, SLOT(onReadyRead()));

    reconnectTimer_->setInterval(3000);
    reconnectTimer_->setSingleShot(true);
    QObject::connect(reconnectTimer_, SIGNAL(timeout()),
                     this, SLOT(onReconnectTimer()));
}

MasterClient::~MasterClient()
{
    disconnectFromMaster();
}

void MasterClient::connectToMaster(const QString& host, int port)
{
    host_ = host;
    port_ = port;
    std::cout << "[qt-client] 连接主站 " << host.toStdString()
              << ":" << port << "...\n";
    socket_->connectToHost(host, static_cast<quint16>(port));
}

void MasterClient::disconnectFromMaster()
{
    reconnectTimer_->stop();
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
    }
}

bool MasterClient::isConnected() const
{
    return connected_;
}

void MasterClient::onConnected()
{
    connected_ = true;
    buffer_.clear();
    reconnectTimer_->stop();
    std::cout << "[qt-client] 已连接主站\n";
    emit connectionStateChanged(true);
}

void MasterClient::onDisconnected()
{
    connected_ = false;
    buffer_.clear();
    std::cout << "[qt-client] 与主站断开，3 秒后重连...\n";
    emit connectionStateChanged(false);

    if (!host_.isEmpty() && port_ > 0) {
        reconnectTimer_->start();
    }
}

void MasterClient::onReadyRead()
{
    buffer_.append(socket_->readAll());

    while (true) {
        int idx = buffer_.indexOf('\n');
        if (idx < 0) break;

        QByteArray line = buffer_.left(idx);
        buffer_.remove(0, idx + 1);

        if (line.endsWith('\r')) line.chop(1);
        if (!line.isEmpty()) processLine(line);
    }
}

void MasterClient::onReconnectTimer()
{
    std::cout << "[qt-client] 尝试重连...\n";
    socket_->connectToHost(host_, static_cast<quint16>(port_));
}

/*
 * 帧分发：根据前缀路由到对应的信号。
 *
 * UPDATE|... → decodeUpdate → telemetryReceived
 * OFFLINE|... → decodeOffline → deviceOffline
 * ONLINE|... → decodeOnline → deviceOnline
 */
void MasterClient::processLine(const QByteArray& line)
{
    std::string frame(line.constData(), static_cast<std::size_t>(line.size()));

    /* OFFLINE 帧 */
    std::string deviceCode;
    if (scada::protocol::decodeOffline(frame, deviceCode)) {
        emit deviceOffline(QString::fromStdString(deviceCode));
        return;
    }

    /* ONLINE 帧 */
    if (scada::protocol::decodeOnline(frame, deviceCode)) {
        emit deviceOnline(QString::fromStdString(deviceCode));
        return;
    }

    /* UPDATE 帧 */
    scada::Telemetry telem;
    if (scada::protocol::decodeUpdate(frame, telem)) {
        emit telemetryReceived(telem);
    } else {
        std::cerr << "[qt-client] 未知帧: " << frame << "\n";
    }
}
