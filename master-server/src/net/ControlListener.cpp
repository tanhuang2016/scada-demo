/**
 * @file   ControlListener.cpp
 * @brief  Qt→主站控制通道实现（端口 5003，RELOAD + CTRL 命令）
 * @module master-server
 */

#include "net/ControlListener.hpp"

#include <cstring>
#include <iostream>

#include "pipeline/CommandRouter.hpp"
#include "scada/protocol.hpp"

namespace master {
namespace net {

ControlListener::ControlListener(int port)
    : port_(port)
    , listenSock_(INVALID_SOCKET)
    , running_(false)
    , reloadRequested_(false)
    , appRunning_(NULL)
    , router_(NULL)
{
}

ControlListener::~ControlListener()
{
    stop();
}

bool ControlListener::start(bool* running, pipeline::CommandRouter* router)
{
    appRunning_ = running;
    router_ = router;

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

bool ControlListener::reloadRequested() const { return reloadRequested_; }
void ControlListener::clearReload() { reloadRequested_ = false; }

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

        char buf[256] = {};
        int n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            std::string cmd(buf, static_cast<std::size_t>(n));
            while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) {
                cmd.pop_back();
            }
            std::cout << "[control] 收到: " << cmd << "\n";

            std::string response = handleCommand(cmd);

            /* 如果命令需要回复，发送响应 */
            if (!response.empty()) {
                response.push_back('\n');
                send(client, response.c_str(),
                     static_cast<int>(response.size()), 0);
            }
        }
        closesocket(client);
    }
}

std::string ControlListener::handleCommand(const std::string& cmd)
{
    /* RELOAD */
    if (cmd == "RELOAD") {
        reloadRequested_ = true;
        if (appRunning_ != NULL) *appRunning_ = false;
        std::cout << "[control] 热加载已触发\n";
        return "";  // 无需回复
    }

    /* CTRL|deviceCode|switchVal */
    if (cmd.size() > 5 && cmd.compare(0, 4, "CTRL") == 0) {
        std::string deviceCode;
        int switchVal = 0;
        if (!scada::protocol::decodeCtrl(cmd, deviceCode, switchVal)) {
            return "CTRL_ACK||0|FAILURE";
        }
        std::string result = "FAILURE";
        if (router_ != NULL) {
            result = router_->handleCtrl(deviceCode, switchVal);
        }
        return scada::protocol::encodeCtrlAck(deviceCode, switchVal,
                                              result == "SUCCESS");
    }

    std::cerr << "[control] 未知命令: " << cmd << "\n";
    return "";
}

}  // namespace net
}  // namespace master
