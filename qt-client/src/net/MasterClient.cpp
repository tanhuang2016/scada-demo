/**
 * @file   MasterClient.cpp
 * @brief  Qt TCP 客户端实现：连接主站 5002 端口，行缓冲 + UPDATE 帧解码
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
    /* 注册 Telemetry 类型以支持 Qt 信号槽传递 */
    qRegisterMetaType<scada::Telemetry>("scada::Telemetry");

    /* 连接 QTcpSocket 信号 */
    QObject::connect(socket_, SIGNAL(connected()),
                     this, SLOT(onConnected()));
    QObject::connect(socket_, SIGNAL(disconnected()),
                     this, SLOT(onDisconnected()));
    QObject::connect(socket_, SIGNAL(readyRead()),
                     this, SLOT(onReadyRead()));

    /* 重连定时器 */
    reconnectTimer_->setInterval(3000);  // 3 秒后重连
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

    /* 启动自动重连定时器 */
    if (!host_.isEmpty() && port_ > 0) {
        reconnectTimer_->start();
    }
}

/*
 * QTcpSocket 数据就绪回调。
 *
 * TCP 是字节流，数据可能分片到达。处理逻辑：
 *   1. 读取全部可用数据追加到 buffer_
 *   2. 按 '\n' 切行
 *   3. 每行调用 processLine 解析 UPDATE 帧
 *   4. 未切完的残片留在 buffer_ 中等待下次 readyRead
 */
void MasterClient::onReadyRead()
{
    buffer_.append(socket_->readAll());

    /* 按行切分（以 \n 为分隔符） */
    while (true) {
        int idx = buffer_.indexOf('\n');
        if (idx < 0) {
            break;  // 没有完整行，等待更多数据
        }

        QByteArray line = buffer_.left(idx);
        buffer_.remove(0, idx + 1);

        /* 去除 \r */
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        if (!line.isEmpty()) {
            processLine(line);
        }
    }
}

void MasterClient::onReconnectTimer()
{
    std::cout << "[qt-client] 尝试重连...\n";
    socket_->connectToHost(host_, static_cast<quint16>(port_));
}

/*
 * 解析一行 UPDATE 帧并发射信号。
 *
 * 帧格式：UPDATE|deviceId|voltage|current|switch|timestamp
 * 使用 common 库的 scada::protocol::decodeUpdate 解析。
 * 解析失败不崩溃，仅打印警告。
 */
void MasterClient::processLine(const QByteArray& line)
{
    std::string frame(line.constData(), static_cast<std::size_t>(line.size()));
    scada::Telemetry telem;

    if (scada::protocol::decodeUpdate(frame, telem)) {
        emit telemetryReceived(telem);
    } else {
        std::cerr << "[qt-client] UPDATE 解析失败: " << frame << "\n";
    }
}
