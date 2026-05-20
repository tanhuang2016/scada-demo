/**
 * @file   JsonTcpServer.cpp
 * @brief  模拟器 TCP 服务端实现（Winsock2，C++11）
 * @module device-simulator
 */

#include "net/JsonTcpServer.hpp"

#include <cstring>
#include <iostream>

namespace simulator {
namespace net {

JsonTcpServer::JsonTcpServer(int port)
    : port_(port)
    , listenSock_(INVALID_SOCKET)
    , clientSock_(INVALID_SOCKET)
{
}

JsonTcpServer::~JsonTcpServer()
{
    stop();
}

bool JsonTcpServer::start()
{
    listenSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock_ == INVALID_SOCKET) {
        std::cerr << "[simulator] socket() 失败: " << WSAGetLastError() << "\n";
        return false;
    }

    /* 不设 SO_REUSEADDR：避免多模拟器实例同时绑定造成混乱 */

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port_));

    if (bind(listenSock_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[simulator] bind() 失败: " << WSAGetLastError() << "\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    if (listen(listenSock_, 1) == SOCKET_ERROR) {
        std::cerr << "[simulator] listen() 失败: " << WSAGetLastError() << "\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    std::cout << "[simulator] TCP 服务已启动，端口: " << port_ << "\n";
    return true;
}

bool JsonTcpServer::waitForClient(int timeoutMs)
{
    if (clientSock_ != INVALID_SOCKET) {
        return true;  // 已有客户端
    }

    if (listenSock_ == INVALID_SOCKET) {
        return false;
    }

    /* 使用 select 实现超时等待 */
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(listenSock_, &readfds);

    struct timeval tv;
    struct timeval* tvPtr = NULL;
    if (timeoutMs > 0) {
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        tvPtr = &tv;
    }

    int ret = select(0, &readfds, NULL, NULL, tvPtr);
    if (ret <= 0) {
        return false;  // 超时或出错
    }

    clientSock_ = accept(listenSock_, NULL, NULL);
    if (clientSock_ == INVALID_SOCKET) {
        std::cerr << "[simulator] accept() 失败: " << WSAGetLastError() << "\n";
        return false;
    }

    std::cout << "[simulator] 主站已连接\n";
    return true;
}

bool JsonTcpServer::sendLine(const std::string& line)
{
    if (clientSock_ == INVALID_SOCKET) {
        return false;
    }

    /* 确保 line 以 \n 结尾 */
    std::string data = line;
    if (data.empty() || data[data.size() - 1] != '\n') {
        data.push_back('\n');
    }

    int total = static_cast<int>(data.size());
    int sent = 0;
    while (sent < total) {
        int n = send(clientSock_, data.c_str() + sent, total - sent, 0);
        if (n == SOCKET_ERROR) {
            std::cerr << "[simulator] send() 失败: " << WSAGetLastError() << "\n";
            return false;
        }
        sent += n;
    }
    return true;
}

void JsonTcpServer::closeClient()
{
    if (clientSock_ != INVALID_SOCKET) {
        closesocket(clientSock_);
        clientSock_ = INVALID_SOCKET;
        std::cout << "[simulator] 客户端连接已关闭\n";
    }
}

void JsonTcpServer::stop()
{
    closeClient();
    if (listenSock_ != INVALID_SOCKET) {
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        std::cout << "[simulator] TCP 服务已停止\n";
    }
}

bool JsonTcpServer::hasClient() const
{
    return clientSock_ != INVALID_SOCKET;
}

}  // namespace net
}  // namespace simulator
