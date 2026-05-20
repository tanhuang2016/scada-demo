#pragma once

/**
 * @file   MasterClient.hpp
 * @brief  Qt TCP 客户端：连接主站 5002 端口，接收 UPDATE 帧，发射遥测信号
 * @module qt-client
 *
 * 使用 QTcpSocket 异步 I/O，不阻塞 Qt 事件循环。
 * 内置自动重连：断开后 3 秒尝试重连。
 */

#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>

// 前向声明 QTcpSocket
class QTcpSocket;

namespace scada {
struct Telemetry;
}  // namespace scada

/**
 * @brief Qt 侧主站客户端——从主站 5002 端口接收实时遥测数据
 *
 * 用法：
 *   1. connectToMaster(host, port) 连接
 *   2. 监听 telemetryReceived(Telemetry) 信号更新 UI
 *   3. 监听 connectionStateChanged(bool) 信号更新连接状态
 */
class MasterClient : public QObject {
    Q_OBJECT

public:
    explicit MasterClient(QObject* parent = 0);
    ~MasterClient();

    /** @brief 连接到主站 */
    void connectToMaster(const QString& host, int port);

    /** @brief 断开连接 */
    void disconnectFromMaster();

    /** @brief 是否已连接 */
    bool isConnected() const;

signals:
    /** 收到一条遥测数据 */
    void telemetryReceived(const scada::Telemetry& telemetry);

    /** 连接状态变化 */
    void connectionStateChanged(bool connected);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onReconnectTimer();

private:
    /** 从缓冲区提取一行，按 UPDATE 帧解析 */
    void processLine(const QByteArray& line);

    QTcpSocket* socket_;
    QTimer* reconnectTimer_;
    QByteArray buffer_;         ///< 接收缓冲区
    QString host_;
    int port_;
    bool connected_;
};

// Telemetry 信号参数注册见 MasterClient.cpp（qRegisterMetaType）
