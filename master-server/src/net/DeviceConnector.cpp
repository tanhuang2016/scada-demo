/**
 * @file   DeviceConnector.cpp
 * @brief  设备连接器实现（Winsock2，C++11）
 *
 * 职责：管理到单台设备（模拟器）的 TCP 连接，按行读取 JSON 帧。
 * 业务层通过 readLine() + protocol()->decodeTelemetry() 获取遥测，不直接拼 JSON。
 *
 * 离线检测采用三层策略（按优先级）：
 *   1. select() 同时监听 readfds + exceptfds — Windows 上远端 RST 可能只置位异常描述符
 *   2. select() 超时后 getsockopt(SO_ERROR) — Windows loopback 上 select 可能一直超时而不报异常
 *   3. 调用层（MasterApplication::runDeviceLoop）连续超时计数器 — 2 秒无数据判定离线
 *
 * Windows Winsock 避坑要点：
 *   - select() 的 exceptfds 不仅用于 OOB 数据，也用于连接异常（RST）
 *     但 loopback 127.0.0.1 上 kill 对端进程时，select 可能不置位任何描述符
 *   - SO_ERROR 能可靠报告挂起的套接字错误，是 Windows 上最可靠的检测手段
 *   - WSAGetLastError() 必须在失败后立即调用，closesocket/getsockopt 等调用可能覆盖它
 *   - WIN32_LEAN_AND_MEAN 保证 winsock2.h 与 windows.h 不冲突（已在根 CMakeLists 全局定义）
 *   - connect() 失败时错误码保存在局部变量再打印，避免 closesocket 覆盖
 *
 * @module master-server
 */

#include "net/DeviceConnector.hpp"

#include <cstring>
#include <iostream>

namespace master {
namespace net {

DeviceConnector::DeviceConnector(const storage::DeviceConfig& config,
                                 std::unique_ptr<scada::device_protocol::IDeviceProtocol> protocol)
    : config_(config)
    , protocol_(std::move(protocol))
    , sock_(INVALID_SOCKET)
    , connected_(false)
{
}

DeviceConnector::~DeviceConnector()
{
    disconnect();
}

bool DeviceConnector::connect()
{
    if (connected_) {
        disconnect();
    }

    sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_ == INVALID_SOCKET) {
        std::cerr << "[master] socket() 失败: " << WSAGetLastError() << "\n";
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(config_.port));

    if (inet_pton(AF_INET, config_.ipAddress.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "[master] 无效 IP: " << config_.ipAddress << "\n";
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
        return false;
    }

    /*
     * connect() 失败时 WSAGetLastError() 必须在 closesocket 之前调用。
     * 原因：closesocket 内部可能触发额外的 Winsock 操作，覆盖错误码。
     * 因此先保存到局部变量 err，再关闭套接字、打印日志。
     */
    if (::connect(sock_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        std::cerr << "[master] 连接失败 " << config_.ipAddress << ":" << config_.port
                  << " (错误: " << err << ")\n";
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
        return false;
    }

    connected_ = true;
    recvBuffer_.clear();
    std::cout << "[master] 已连接设备 " << config_.deviceCode
              << " (" << config_.ipAddress << ":" << config_.port << ")\n";
    return true;
}

void DeviceConnector::disconnect()
{
    if (sock_ != INVALID_SOCKET) {
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
    connected_ = false;
    recvBuffer_.clear();
}

/*
 * 发送一行数据到设备（线程安全）。
 * TCP 是全双工的——recv 和 send 可以在不同线程同时进行。
 */
bool DeviceConnector::sendLine(const std::string& line)
{
    std::lock_guard<std::mutex> lock(sendMutex_);
    if (!connected_ || sock_ == INVALID_SOCKET) return false;

    std::string data = line;
    if (data.empty() || data[data.size() - 1] != '\n') {
        data.push_back('\n');
    }

    int total = static_cast<int>(data.size());
    int sent = 0;
    while (sent < total) {
        int n = send(sock_, data.c_str() + sent, total - sent, 0);
        if (n == SOCKET_ERROR) {
            std::cerr << "[" << config_.deviceCode << "] sendLine 失败: "
                      << WSAGetLastError() << "\n";
            return false;
        }
        sent += n;
    }
    return true;
}

bool DeviceConnector::isConnected() const
{
    return connected_;
}

/*
 * 按行读取（阻塞式 + select 超时）。
 *
 * TCP 是字节流，不是消息流。接收过程：
 *   1. 先检查 recvBuffer_ 是否已有完整行（含 \n）
 *      有 → 弹出返回；没有 → 进入轮询
 *   2. select() 等待可读或异常
 *   3. 收到数据 → recv() → append 到 recvBuffer_ → 再次检查 \n
 *   4. 收到完整行 → 弹出返回；否则继续 select
 *
 * 离线检测三层策略（Windows 特有坑）：
 *   [第1层] select exceptfds — 远端发 RST 时，Windows 可能将其放入 exceptfds
 *           而非 readfds。POSIX 兼容的 select 通常把 RST 放在 readfds。
 *   [第2层] SO_ERROR — select 超时不代表连接正常。Windows loopback 127.0.0.1
 *           上 kill 对端进程后，select 可能持续超时而不置位任何 fd。
 *           SO_ERROR 由内核维护，能可靠拿到 WSAECONNRESET (10054) 等错误。
 *   [第3层] 调用方超时计数器 — MasterApplication::runDeviceLoop 中 kMaxNoData
 *           兜底。若以上两层均未触发，累计 2 秒无数据即主动断开。
 */
bool DeviceConnector::readLine(std::string& line, int timeoutMs)
{
    /* 优先消费缓冲区中已有的完整行，避免不必要的 select/recv 系统调用 */
    std::size_t pos = recvBuffer_.find('\n');
    if (pos != std::string::npos) {
        line = recvBuffer_.substr(0, pos);
        recvBuffer_.erase(0, pos + 1);
        /* 去除行尾 \r（Windows/DOS 风格换行 CRLF） */
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        return true;
    }

    /* 缓冲区无完整行，进入 select + recv 轮询 */
    while (true) {
        /*
         * select() 同时监控 readfds 和 exceptfds。
         * - readfds：数据可读
         * - exceptfds：OOB 数据或连接异常（Windows 扩展语义包含 RST）
         *
         * 注意：select 的第 1 个参数在 Windows 上被忽略，
         * 仅 POSIX 用于指定 nfds 上限。
         */
        fd_set readfds;
        fd_set exceptfds;
        FD_ZERO(&readfds);
        FD_ZERO(&exceptfds);
        FD_SET(sock_, &readfds);
        FD_SET(sock_, &exceptfds);

        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        int ret = select(0, &readfds, NULL, &exceptfds, &tv);
        if (ret == 0) {
            /*
             * 超时 — 不代表连接正常！
             *
             * Windows 127.0.0.1 loopback 已知问题：
             * 当远端进程被 force kill（taskkill /F 或关闭终端窗口），
             * TCP 发送 RST，但 select() 在 loopback 接口上可能既不置位
             * readfds 也不置位 exceptfds，只持续超时。
             *
             * 因此每次超时都用 getsockopt(SO_ERROR) 显式检查。
             * SO_ERROR 是内核维护的套接字错误状态，
             * 一旦收到 RST，此值会变为 WSAECONNRESET (10054) 等。
             * getsockopt 后内核会自动清零 SO_ERROR。
             */
            int err = 0;
            int errlen = sizeof(err);
            getsockopt(sock_, SOL_SOCKET, SO_ERROR,
                       reinterpret_cast<char*>(&err), &errlen);
            if (err != 0) {
                std::cerr << "[master] SO_ERROR=" << err
                          << "，远端已断开 (检测点: select超时后SO_ERROR检查)\n";
                connected_ = false;
                return false;
            }
            /* 真正的超时 — 无数据但连接正常 */
            return false;
        }
        if (ret == SOCKET_ERROR) {
            std::cerr << "[master] select() 失败: " << WSAGetLastError() << "\n";
            connected_ = false;
            return false;
        }

        /*
         * 第 1 层检测：select 返回且 exceptfds 被置位。
         * Windows 上远端 RST 优先走此路径。
         * getsockopt(SO_ERROR) 获取具体错误码（通常 WSAECONNRESET）。
         */
        if (FD_ISSET(sock_, &exceptfds)) {
            int err = 0;
            int errlen = sizeof(err);
            getsockopt(sock_, SOL_SOCKET, SO_ERROR,
                       reinterpret_cast<char*>(&err), &errlen);
            std::cerr << "[master] 套接字异常: " << err
                      << " (检测点: select exceptfds 置位)\n";
            connected_ = false;
            return false;
        }

        /* 第 2 层检测：readfds 置位，尝试 recv */
        char buf[4096];
        int n = recv(sock_, buf, sizeof(buf), 0);
        if (n <= 0) {
            /*
             * n == 0：对端优雅关闭（FIN），连接正常终止
             * n < 0：出错（通常 WSAECONNRESET），对端强制关闭
             */
            if (n < 0) {
                std::cerr << "[master] recv() 错误: " << WSAGetLastError()
                          << " (检测点: readfds置位但recv失败)\n";
            }
            connected_ = false;
            return false;
        }

        /* 将新数据追加到缓冲区 */
        recvBuffer_.append(buf, static_cast<std::size_t>(n));

        /* 再次检查是否收到完整行（可能一次 recv 收到多行） */
        pos = recvBuffer_.find('\n');
        if (pos != std::string::npos) {
            line = recvBuffer_.substr(0, pos);
            recvBuffer_.erase(0, pos + 1);
            if (!line.empty() && line[line.size() - 1] == '\r') {
                line.erase(line.size() - 1);
            }
            return true;
        }
        /*
         * 还没有完整行（数据被 TCP 分片或一次 recv 未收完），
         * 继续 select() 等待剩余数据。
         * 注意：此时超时起点从 select 调用开始重新计算，
         * 并非累计原 timeoutMs。
         */
    }
}

scada::device_protocol::IDeviceProtocol* DeviceConnector::protocol() const
{
    return protocol_.get();
}

const storage::DeviceConfig& DeviceConnector::config() const
{
    return config_;
}

}  // namespace net
}  // namespace master
