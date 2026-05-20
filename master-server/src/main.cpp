/**
 * @file   main.cpp
 * @brief  主站进程入口：Winsock 初始化 + 信号处理 + MasterApplication
 */

/* winsock2.h 必须在 windows.h 之前包含 */
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "app/MasterApplication.hpp"

#include <csignal>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

master::MasterApplication* g_app = 0;

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
    /* 设置控制台编码为 UTF-8 */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    /* 初始化 Winsock */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[master-server] WSAStartup 失败\n";
        return 1;
    }
#endif

    master::MasterApplication app;
    g_app = &app;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const int code = app.run();
    g_app = 0;

#ifdef _WIN32
    WSACleanup();
#endif

    return code;
}
