#pragma once

/**
 * @file   UiBroadcaster.hpp
 * @brief  主站→Qt 实时数据推送服务（TCP 端口 5002）
 * @module master-server
 *
 * 职责：接受 Qt 客户端连接，将 Telemetry 编码为 UPDATE 帧发送。
 *
 * 线程模型：
 *   - 内部独立线程运行 accept 循环（select + 超时）
 *   - broadcast() 由业务线程调用，互斥锁保护 clientSock_ 的读写
 *   - 同一时间只服务一个 Qt 客户端（迭代 4 可扩展为多客户端）
 */

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace scada {
struct Telemetry;
}  // namespace scada

namespace master {
namespace push {

class UiBroadcaster {
public:
    /**
     * @param port 监听端口（通常为 kMasterToUiPort = 5002）
     */
    explicit UiBroadcaster(int port);

    ~UiBroadcaster();

    /** @brief 启动内部线程，开始监听 */
    bool start();

    /** @brief 停止线程并关闭套接字 */
    void stop();

    /**
     * @brief 广播一条遥测到已连接的 Qt 客户端（线程安全）
     * @param telem  遥测快照，内部调用 scada::protocol::encodeUpdate
     *
     * 如果当前无客户端连接，静默丢弃。
     * 如果 send 失败，关闭客户端并等待重连。
     */
    void broadcast(const scada::Telemetry& telem);

    /**
     * @brief 发送任意文本行到 Qt 客户端（线程安全）
     *
     * 用于推送非遥测帧（如 OFFLINE、ONLINE 等控制消息）。
     * 如果 line 不以 \n 结尾，自动追加。
     */
    void sendLine(const std::string& line);

    /** @brief 当前是否有 Qt 客户端连接 */
    bool hasClient() const;

private:
    /** accept 循环（在独立线程中运行） */
    void runAcceptLoop();

    /** 关闭当前客户端连接 */
    void closeClient();

    int port_;
    std::thread thread_;
    std::mutex mutex_;
    SOCKET listenSock_;
    SOCKET clientSock_;
    std::atomic<bool> running_;
};

}  // namespace push
}  // namespace master
