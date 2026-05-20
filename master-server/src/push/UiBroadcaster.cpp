/**
 * @file   UiBroadcaster.cpp
 * @brief  主站→Qt 推送服务实现（Winsock2 TCP 服务端，端口 5002）
 * @module master-server
 */

#include "push/UiBroadcaster.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "scada/protocol.hpp"
#include "scada/types.hpp"

namespace master {
namespace push {

UiBroadcaster::UiBroadcaster(int port)
    : port_(port)
    , listenSock_(INVALID_SOCKET)
    , clientSock_(INVALID_SOCKET)
    , running_(false)
{
}

UiBroadcaster::~UiBroadcaster()
{
    stop();
}

bool UiBroadcaster::start()
{
    if (running_) return true;

    listenSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock_ == INVALID_SOCKET) {
        std::cerr << "[broadcaster] socket() 失败: " << WSAGetLastError() << "\n";
        return false;
    }

    /*
     * 不设 SO_REUSEADDR——原因同 JsonTcpServer：
     * 避免多实例同时 bind 造成端口混乱和僵尸连接。
     */
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port_));

    if (bind(listenSock_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[broadcaster] bind() 失败 (端口 " << port_
                  << "): " << WSAGetLastError() << "\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    if (listen(listenSock_, 1) == SOCKET_ERROR) {
        std::cerr << "[broadcaster] listen() 失败: " << WSAGetLastError() << "\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    running_ = true;
    thread_ = std::thread(&UiBroadcaster::runAcceptLoop, this);
    std::cout << "[broadcaster] 推送服务已启动，端口: " << port_ << "\n";
    return true;
}

void UiBroadcaster::stop()
{
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
    closeClient();
    if (listenSock_ != INVALID_SOCKET) {
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
    }
}

void UiBroadcaster::broadcast(const scada::Telemetry& telem)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (clientSock_ == INVALID_SOCKET) {
        return;  // 无客户端，静默丢弃
    }

    std::string frame = scada::protocol::encodeUpdate(telem);
    frame.push_back('\n');

    int total = static_cast<int>(frame.size());
    int sent = 0;
    while (sent < total) {
        int n = send(clientSock_, frame.c_str() + sent, total - sent, 0);
        if (n == SOCKET_ERROR) {
            std::cerr << "[broadcaster] send() 失败: " << WSAGetLastError()
                      << "，断开客户端\n";
            /* 在锁内调用 closeClient 不安全（需要获取已持有的锁），
             * 因此直接关闭并置无效 */
            closesocket(clientSock_);
            clientSock_ = INVALID_SOCKET;
            return;
        }
        sent += n;
    }
}

bool UiBroadcaster::hasClient() const
{
    return clientSock_ != INVALID_SOCKET;
}

void UiBroadcaster::runAcceptLoop()
{
    std::cout << "[broadcaster] 等待 Qt 客户端连接...\n";

    while (running_) {
        if (clientSock_ != INVALID_SOCKET) {
            /*
             * 已有客户端，不需要再次 accept。
             * 休眠等待客户端断开或 stop()。
             * 注意：客户端断连由 broadcast() 检测（send 失败时关闭）。
             */
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        /* select 可读表示有新连接到达 */
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenSock_, &readfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;  // 500ms

        int ret = select(0, &readfds, NULL, NULL, &tv);
        if (ret <= 0) {
            continue;  // 超时或出错
        }

        std::lock_guard<std::mutex> lock(mutex_);
        clientSock_ = accept(listenSock_, NULL, NULL);
        if (clientSock_ != INVALID_SOCKET) {
            std::cout << "[broadcaster] Qt 客户端已连接\n";
        }
    }

    std::cout << "[broadcaster] accept 循环已退出\n";
}

void UiBroadcaster::closeClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (clientSock_ != INVALID_SOCKET) {
        closesocket(clientSock_);
        clientSock_ = INVALID_SOCKET;
        std::cout << "[broadcaster] Qt 客户端连接已关闭\n";
    }
}

}  // namespace push
}  // namespace master
