/**
 * @file   ControlListener.cpp
 * @brief  Qt→主站控制通道实现（端口 5003，接收 RELOAD 命令）
 * @module master-server
 */

#include "net/ControlListener.hpp"

#include <cstring>
#include <iostream>

namespace master {
namespace net {

ControlListener::ControlListener(int port)
    : port_(port)
    , listenSock_(INVALID_SOCKET)
    , running_(false)
    , reloadRequested_(false)
    , appRunning_(NULL)
{
}

ControlListener::~ControlListener()
{
    stop();
}

bool ControlListener::start(bool* running)
{
    appRunning_ = running;
    listenSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock_ == INVALID_SOCKET) {
        std::cerr << "[control] socket() 失败: " << WSAGetLastError() << "\n";
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port_));

    if (bind(listenSock_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[control] bind() 失败: " << WSAGetLastError() << "\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    if (listen(listenSock_, 1) == SOCKET_ERROR) {
        std::cerr << "[control] listen() 失败: " << WSAGetLastError() << "\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    running_ = true;
    thread_ = std::thread(&ControlListener::runLoop, this);
    std::cout << "[control] 控制通道已启动，端口: " << port_ << "\n";
    return true;
}

void ControlListener::stop()
{
    running_ = false;
    if (thread_.joinable()) thread_.join();
    if (listenSock_ != INVALID_SOCKET) {
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
    }
}

bool ControlListener::reloadRequested() const
{
    return reloadRequested_;
}

void ControlListener::clearReload()
{
    reloadRequested_ = false;
}

void ControlListener::runLoop()
{
    while (running_) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenSock_, &readfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;

        int ret = select(0, &readfds, NULL, NULL, &tv);
        if (ret <= 0) continue;

        SOCKET client = accept(listenSock_, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        /* 读取一行（最多 256 字节） */
        char buf[256] = {};
        int n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            std::string cmd(buf, static_cast<std::size_t>(n));
            /* 去除 \r\n */
            while (!cmd.empty() && (cmd[cmd.size() - 1] == '\n' || cmd[cmd.size() - 1] == '\r')) {
                cmd.erase(cmd.size() - 1);
            }
            std::cout << "[control] 收到命令: " << cmd << "\n";
            if (cmd == "RELOAD") {
                reloadRequested_ = true;
                if (appRunning_ != NULL) {
                    *appRunning_ = false;  // 停止设备线程
                }
                std::cout << "[control] 热加载已触发\n";
            }
        }
        closesocket(client);
    }
}

}  // namespace net
}  // namespace master
