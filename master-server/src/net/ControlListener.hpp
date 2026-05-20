#pragma once

/**
 * @file   ControlListener.hpp
 * @brief  Qt→主站控制通道（TCP 端口 5003），接收 RELOAD / CTRL 命令
 * @module master-server
 *
 * 命令格式（一行一条）：
 *   RELOAD              — 请求主站热加载 MySQL 设备配置
 *   CTRL|deviceCode|0/1  — 遥控分闸/合闸
 */

#include <atomic>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace master {
namespace pipeline {
class CommandRouter;
}

namespace net {

class ControlListener {
public:
    explicit ControlListener(int port);
    ~ControlListener();

    /** @param running  指向 MasterApplication::running_（RELOAD 时置 false） */
    bool start(bool* running, pipeline::CommandRouter* router = NULL);
    void stop();

    bool reloadRequested() const;
    void clearReload();

private:
    void runLoop();

    /** @brief 处理一条命令，返回响应（CTRL 命令需要回复） */
    std::string handleCommand(const std::string& cmd);

    int port_;
    std::thread thread_;
    SOCKET listenSock_;
    std::atomic<bool> running_;
    std::atomic<bool> reloadRequested_;
    bool* appRunning_;
    pipeline::CommandRouter* router_;
};

}  // namespace net
}  // namespace master
