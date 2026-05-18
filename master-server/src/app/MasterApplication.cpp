/**
 * @file   MasterApplication.cpp
 * @brief  主站应用实现（当前为脚手架占位循环）
 */

#include "app/MasterApplication.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include "scada/config_defaults.hpp"

namespace master {

MasterApplication::MasterApplication()
    : running_(false)
{
}

MasterApplication::~MasterApplication()
{
}

int MasterApplication::run()
{
    running_ = true;
    std::cout << "[master-server] 启动（脚手架）\n"
              << "  IEC104 设备端口: " << scada::config::kDeviceToMasterPort << '\n'
              << "  UI 推送端口: " << scada::config::kMasterToUiPort << '\n'
              << "  UI 控制端口: " << scada::config::kUiToMasterPort << '\n';

  /* 迭代 1：在此加载 MySQL；迭代 2：启动 104；迭代 3：启动 UiBroadcaster */
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[master-server] 已停止\n";
    return 0;
}

void MasterApplication::requestStop()
{
    running_ = false;
}

}  // namespace master
