#include "app/SimulatorApplication.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include "scada/config_defaults.hpp"
#include "scada/protocol.hpp"
#include "scada/types.hpp"

namespace simulator {

SimulatorApplication::SimulatorApplication() = default;
SimulatorApplication::~SimulatorApplication() = default;

int SimulatorApplication::run()
{
    running_ = true;
    std::cout << "[device-simulator] starting (stub)\n"
              << "  target master port: " << scada::config::kDeviceToMasterPort << '\n';

    scada::Telemetry sample;
    sample.deviceId = "dev01";
    sample.voltage = 220.0;
    sample.current = 10.0;
    sample.switchState = scada::SwitchState::Closed;
    sample.timestamp = 0;
    std::cout << "  sample frame: " << scada::protocol::encodeTelemetry(sample) << '\n';

    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[device-simulator] stopped\n";
    return 0;
}

void SimulatorApplication::requestStop()
{
    running_ = false;
}

}  // namespace simulator
