/**
 * @file   DeviceConnector.cpp
 * @brief  设备连接器实现（Winsock2，C++11）
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

    if (::connect(sock_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();  // 立即保存，避免 closesocket 覆盖
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

bool DeviceConnector::isConnected() const
{
    return connected_;
}

bool DeviceConnector::readLine(std::string& line, int timeoutMs)
{
    /* 先检查缓冲区是否已有完整行 */
    std::size_t pos = recvBuffer_.find('\n');
    if (pos != std::string::npos) {
        line = recvBuffer_.substr(0, pos);
        recvBuffer_.erase(0, pos + 1);
        /* 去除 \r */
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        return true;
    }

    /* 缓冲区不足，等待网络数据 */
    while (true) {
        /* Windows select: 须同时检查 exceptfds 捕获远端 RST */
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
            /* 超时：用 SO_ERROR 显式探测远端是否已断开
             * （Windows loopback 上 select 可能不报异常） */
            int err = 0;
            int errlen = sizeof(err);
            getsockopt(sock_, SOL_SOCKET, SO_ERROR,
                       reinterpret_cast<char*>(&err), &errlen);
            if (err != 0) {
                std::cerr << "[master] SO_ERROR=" << err << "，远端已断开\n";
                connected_ = false;
                return false;
            }
            return false;  // 真正的超时
        }
        if (ret == SOCKET_ERROR) {
            std::cerr << "[master] select() 失败: " << WSAGetLastError() << "\n";
            connected_ = false;
            return false;
        }

        /* 远端发送 RST 时，Windows 可能只置位 exceptfds */
        if (FD_ISSET(sock_, &exceptfds)) {
            int err = 0;
            int errlen = sizeof(err);
            getsockopt(sock_, SOL_SOCKET, SO_ERROR,
                       reinterpret_cast<char*>(&err), &errlen);
            std::cerr << "[master] 套接字异常: " << err << "\n";
            connected_ = false;
            return false;
        }

        /* 有数据可读 */
        char buf[4096];
        int n = recv(sock_, buf, sizeof(buf), 0);
        if (n <= 0) {
            /* 连接关闭（n==0）或出错（n<0） */
            if (n < 0) {
                std::cerr << "[master] recv() 错误: " << WSAGetLastError() << "\n";
            }
            connected_ = false;
            return false;
        }

        recvBuffer_.append(buf, static_cast<std::size_t>(n));

        /* 再次检查是否有完整行 */
        pos = recvBuffer_.find('\n');
        if (pos != std::string::npos) {
            line = recvBuffer_.substr(0, pos);
            recvBuffer_.erase(0, pos + 1);
            if (!line.empty() && line[line.size() - 1] == '\r') {
                line.erase(line.size() - 1);
            }
            return true;
        }
        /* 没有完整行，继续等待（使用剩余超时） */
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
