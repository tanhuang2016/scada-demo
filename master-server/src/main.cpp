/**
 * @file   main.cpp
 * @brief  主站进程入口：Winsock 初始化 + 信号处理 + MasterApplication
 *
 * Windows Winsock 注意事项：
 *   1. winsock2.h 必须在 windows.h 之前包含（避免与旧版 winsock.h 冲突）
 *      WIN32_LEAN_AND_MEAN 在根 CMakeLists 全局定义，进一步防止冲突
 *   2. WSAStartup 须在程序入口调用一次（不可在库的静态构造中调用，
 *      因为静态初始化顺序不确定）
 *   3. WSACleanup 在退出前调用，释放 Winsock 资源
 *   4. SetConsoleOutputCP(CP_UTF8) 解决 Windows 控制台中文乱码
 */

/* winsock2.h 必须第一个 include，防止与后续 windows.h 冲突 */
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

/*
 * 信号处理函数：设置退出标志。
 *
 * 注意：信号处理函数中只能安全操作 volatile / atomic 变量和简单赋值。
 * requestStop() 内部只是 setting running_ = false，符合信号安全要求。
 */
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
    /* Windows 控制台 UTF-8：避免中文乱码 */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    /*
     * Winsock 初始化（必须在任何 socket 调用之前）。
     *
     * MAKEWORD(2,2)：请求 Winsock 2.2 版本。
     * 失败时打印错误并退出，不要继续执行（后续 socket 调用会崩溃）。
     */
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
    /* 释放 Winsock 资源，与 WSAStartup 配对 */
    WSACleanup();
#endif

    return code;
}
