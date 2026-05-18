#include "app/MasterApplication.hpp"

#include <csignal>

namespace {

master::MasterApplication* g_app = nullptr;

void handleSignal(int)
{
    if (g_app != nullptr) {
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
    g_app = nullptr;
    return code;
}
