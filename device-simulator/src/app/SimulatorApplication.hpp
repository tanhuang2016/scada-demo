#pragma once

namespace simulator {

class SimulatorApplication {
public:
    SimulatorApplication();
    ~SimulatorApplication();

    SimulatorApplication(const SimulatorApplication&) = delete;
    SimulatorApplication& operator=(const SimulatorApplication&) = delete;

    int run();
    void requestStop();

private:
    bool running_ = false;
};

}  // namespace simulator
