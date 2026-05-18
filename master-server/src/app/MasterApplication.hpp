#pragma once

namespace master {

class MasterApplication {
public:
    MasterApplication();
    ~MasterApplication();

    MasterApplication(const MasterApplication&) = delete;
    MasterApplication& operator=(const MasterApplication&) = delete;

    int run();
    void requestStop();

private:
    bool running_ = false;
};

}  // namespace master
