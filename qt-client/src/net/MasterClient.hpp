#pragma once

/**
 * @file   MasterClient.hpp
 * @brief  Qt TCP 客户端：连接主站 5002 端口，接收 UPDATE / OFFLINE / ONLINE 帧
 * @module qt-client
 *
 * 帧类型：
 *   UPDATE|deviceId|voltage|current|switch|ts  → 遥测数据
 *   OFFLINE|deviceId                            → 设备离线
 *   ONLINE|deviceId                             → 设备上线
 *
 * 使用 QTcpSocket 异步 I/O，内置 3 秒自动重连。
 */

#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>

class QTcpSocket;

namespace scada {
struct Telemetry;
}  // namespace scada

class MasterClient : public QObject {
    Q_OBJECT

public:
    explicit MasterClient(QObject* parent = 0);
    ~MasterClient();

    void connectToMaster(const QString& host, int port);
    void disconnectFromMaster();
    bool isConnected() const;

signals:
    /** 收到遥测数据 */
    void telemetryReceived(const scada::Telemetry& telemetry);

    /** 主站连接状态变化（TCP 链路层面） */
    void connectionStateChanged(bool connected);

    /** 某台设备上线 */
    void deviceOnline(const QString& deviceId);

    /** 某台设备离线 */
    void deviceOffline(const QString& deviceId);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onReconnectTimer();

private:
    void processLine(const QByteArray& line);

    QTcpSocket* socket_;
    QTimer* reconnectTimer_;
    QByteArray buffer_;
    QString host_;
    int port_;
    bool connected_;
};
