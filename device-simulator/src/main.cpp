#include "app/SimulatorApplication.hpp"

#include <csignal>

namespace {

simulator::SimulatorApplication* g_app = nullptr;

void handleSignal(int)
{
    if (g_app != nullptr) {
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
    g_app = nullptr;
    return code;
}
