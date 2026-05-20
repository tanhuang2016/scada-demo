#pragma once

/**
 * @file   JsonTcpServer.hpp
 * @brief  模拟器 TCP 服务端：监听端口，接受主站连接，发送 JSON 遥测行
 * @module device-simulator
 */

#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace simulator {
namespace net {

/**
 * @brief 模拟器侧 TCP 服务端，监听指定端口并接受主站连接
 *
 * 生命周期：
 *   1. start() → bind + listen
 *   2. waitForClient() → accept（可带超时）
 *   3. sendLine() → 发送一行 JSON 遥测
 *   4. closeClient() / stop() → 关闭连接
 */
class JsonTcpServer {
public:
    explicit JsonTcpServer(int port);

    ~JsonTcpServer();

    /** @brief 创建套接字、bind、listen */
    bool start();

    /**
     * @brief 等待主站连接（阻塞，可设超时）
     * @param timeoutMs  超时毫秒，0 表示无限等待
     * @return true 表示有客户端连入
     */
    bool waitForClient(int timeoutMs = 0);

    /** @brief 发送一行数据（自动追加换行符） */
    bool sendLine(const std::string& line);

    /** @brief 关闭当前客户端连接 */
    void closeClient();

    /** @brief 关闭监听套接字和客户端连接 */
    void stop();

    /** @brief 读取一行数据（带超时），用于接收遥控等下行命令 */
    bool readLine(std::string& line, int timeoutMs = 100);

    /** @brief 是否有客户端连接 */
    bool hasClient() const;

private:
    int port_;
    SOCKET listenSock_;
    SOCKET clientSock_;
};

}  // namespace net
}  // namespace simulator
