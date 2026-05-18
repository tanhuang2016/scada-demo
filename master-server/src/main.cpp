/**
 * @file   main.cpp
 * @brief  主站进程入口：注册信号处理并运行 MasterApplication
 */

#include "app/MasterApplication.hpp"

#include <csignal>

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
    master::MasterApplication app;
    g_app = &app;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const int code = app.run();
    g_app = 0;
    return code;
}
