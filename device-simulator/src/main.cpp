/**
 * @file   main.cpp
 * @brief  设备模拟器入口：Winsock 初始化 + 信号处理 + SimulatorApplication
 *
 * Winsock 初始化要求详见 master-server/src/main.cpp 的同名注释。
 */

#include "app/SimulatorApplication.hpp"

#include <csignal>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace {

simulator::SimulatorApplication* g_app = 0;

void handleSignal(int)
{
    if (g_app != 0) {
        g_app->requestStop();
    }
}

}  // namespace

int main()
{
#ifdef _WIN32
    /* Windows 控制台 UTF-8 */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    /* Winsock 初始化 — 必须在 socket 调用之前 */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[device-simulator] WSAStartup 失败\n";
        return 1;
    }
#endif

    simulator::SimulatorApplication app;
    g_app = &app;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const int code = app.run();

    g_app = 0;

#ifdef _WIN32
    /* 释放 Winsock 资源 */
    WSACleanup();
#endif

    return code;
}
