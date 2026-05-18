#include "app/MasterApplication.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include "scada/config_defaults.hpp"

namespace master {

MasterApplication::MasterApplication() = default;
MasterApplication::~MasterApplication() = default;

int MasterApplication::run()
{
    running_ = true;
    std::cout << "[master-server] starting (stub)\n"
              << "  device port: " << scada::config::kDeviceToMasterPort << '\n'
              << "  ui push port: " << scada::config::kMasterToUiPort << '\n'
              << "  ui ctrl port: " << scada::config::kUiToMasterPort << '\n';

    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[master-server] stopped\n";
    return 0;
}

void MasterApplication::requestStop()
{
    running_ = false;
}

}  // namespace master
