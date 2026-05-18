/**
 * @file   main.cpp
 * @brief  设备模拟器入口：信号处理 + SimulatorApplication
 */

#include "app/SimulatorApplication.hpp"

#include <csignal>

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
    simulator::SimulatorApplication app;
    g_app = &app;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const int code = app.run();
    g_app = 0;
    return code;
}
