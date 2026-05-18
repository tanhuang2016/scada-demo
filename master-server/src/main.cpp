/**
 * @file   main.cpp
 * @brief  主站进程入口：注册信号处理并运行 MasterApplication
 */

#include "app/MasterApplication.hpp"

#include <csignal>

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
    // 设置 Windows 控制台编码为 UTF-8，解决中文乱码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    master::MasterApplication app;
    g_app = &app;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const int code = app.run();
    g_app = 0;
    return code;
}
