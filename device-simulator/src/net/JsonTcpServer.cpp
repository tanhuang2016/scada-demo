/**
 * @file   JsonTcpServer.cpp
 * @brief  模拟器 TCP 服务端实现（Winsock2，C++11）
 *
 * 职责：监听端口，接受主站 TCP 连接，逐行发送 JSON 遥测帧。
 *
 * SO_REUSEADDR 避坑说明：
 *   - 本实现故意不设置 SO_REUSEADDR。
 *   - 原因：如果设置，多个模拟器实例可同时 bind 同一端口，
 *     导致只有其中一个能 accept，其余僵尸进程占用端口不释放。
 *     测试时容易堆积大量不可杀进程，主站 connect() 可能路由到错误实例。
 *   - 去掉后：新实例启动时如果端口被占用，bind() 直接失败并报错，
 *     用户可立即发现问题，kill 旧进程后重试。
 *   - 快速重启场景（< TIME_WAIT 周期约 120s）如需复用，
 *     可在 bind 失败时打清晰日志提示用户等待或手动杀进程。
 *
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

    /*
     * 不设置 SO_REUSEADDR（详见文件头注释）。
     * 如果端口已被占用，bind 会直接失败，便于排查僵尸进程。
     */

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 监听所有网络接口
    addr.sin_port = htons(static_cast<u_short>(port_));

    if (bind(listenSock_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[simulator] bind() 失败: " << WSAGetLastError()
                  << " (端口 " << port_ << " 可能被占用)\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    /* backlog = 1：当前迭代只接受一台主站连接，多设备在迭代4扩展 */
    if (listen(listenSock_, 1) == SOCKET_ERROR) {
        std::cerr << "[simulator] listen() 失败: " << WSAGetLastError() << "\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    std::cout << "[simulator] TCP 服务已启动，端口: " << port_ << "\n";
    return true;
}

/*
 * 等待主站连接（通过 select 实现超时）。
 *
 * 不使用阻塞 accept 的原因：
 *   SimulatorApplication 的主循环需要定期检查 running_ 以响应 Ctrl+C。
 *   如果 accept 无限阻塞，用户按 Ctrl+C 后需等到有客户端连接才能退出。
 *   select 配合 1 秒超时 = 每秒醒来检查一次 running_。
 */
bool JsonTcpServer::waitForClient(int timeoutMs)
{
    if (clientSock_ != INVALID_SOCKET) {
        return true;  // 已有客户端
    }

    if (listenSock_ == INVALID_SOCKET) {
        return false;
    }

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

/*
 * 发送一行数据（确保以 \n 结尾）。
 *
 * send() 不一定一次发送完所有数据（TCP 发送缓冲区可能满），
 * 因此用循环 + 偏移量确保全部发送。
 */
bool JsonTcpServer::sendLine(const std::string& line)
{
    if (clientSock_ == INVALID_SOCKET) {
        return false;
    }

    /* 确保 line 以 \n 结尾（JSON 行协议分隔符） */
    std::string data = line;
    if (data.empty() || data[data.size() - 1] != '\n') {
        data.push_back('\n');
    }

    int total = static_cast<int>(data.size());
    int sent = 0;
    while (sent < total) {
        int n = send(clientSock_, data.c_str() + sent, total - sent, 0);
        if (n == SOCKET_ERROR) {
            /*
             * send 失败通常是客户端断连（WSAECONNRESET）。
             * 不在这里 closeClient——由调用方（SimulatorApplication）
             * 检查返回值后调用 closeClient，保持职责分离。
             */
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
