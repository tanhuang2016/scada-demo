#pragma once

/**
 * @file   ControlListener.hpp
 * @brief  Qt→主站控制通道（TCP 端口 5003），接收 RELOAD 等控制命令
 * @module master-server
 *
 * 命令格式（一行一条）：
 *   RELOAD  — 请求主站重新加载 MySQL 设备配置并热重连
 *
 * 独立线程运行，accept 一个连接 → 读一行 → 处理 → 断开 → 继续 accept。
 */

#include <atomic>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace master {
namespace net {

class ControlListener {
public:
    explicit ControlListener(int port);
    ~ControlListener();

    /** @param running 指向主循环运行标志，RELOAD 时置 false */
    bool start(bool* running);
    void stop();

    bool reloadRequested() const;
    void clearReload();

private:
    void runLoop();

    int port_;
    std::thread thread_;
    SOCKET listenSock_;
    std::atomic<bool> running_;
    std::atomic<bool> reloadRequested_;
    bool* appRunning_;  ///< 指向 MasterApplication::running_
};

}  // namespace net
}  // namespace master
