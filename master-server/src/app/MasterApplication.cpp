/**
 * @file   MasterApplication.cpp
 * @brief  主站应用实现（迭代1：MySQL基座）
 */

#include "app/MasterApplication.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <iomanip>

#include "scada/config_defaults.hpp"
#include "storage/DeviceRepository.hpp"

#ifndef USE_MOCK_STORAGE
#include "storage/MySQLConnection.hpp"
#endif

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
    std::cout << "[master-server] 启动" << std::endl
              << "  IEC104 设备端口: " << scada::config::kDeviceToMasterPort << std::endl
              << "  UI 推送端口: " << scada::config::kMasterToUiPort << std::endl
              << "  UI 控制端口: " << scada::config::kUiToMasterPort << std::endl;

#ifdef USE_MOCK_STORAGE
    std::cout << "  配置模式: 模拟（无 MySQL 依赖）" << std::endl;
#else
    // 迭代1：加载 MySQL 配置
    storage::MySQLConfig mysqlConfig;
    mysqlConfig.host = "127.0.0.1";
    mysqlConfig.port = 3306;
    mysqlConfig.username = "root";
    mysqlConfig.password = "tanhuang";
    mysqlConfig.database = "scada_demo";

    // 初始化 MySQL 连接
    storage::MySQLConnection::instance().initialize(mysqlConfig);

    // 检查连接有效性（允许失败继续运行，但打印警告）
    bool mysqlOk = storage::MySQLConnection::instance().isValid();
    std::cout << "  配置模式: 真实 MySQL (连接状态: " << (mysqlOk ? "OK" : "FAILED") << ")" << std::endl;
#endif

    // 加载并打印设备配置（不管 MySQL 连接是否成功，都尝试加载）
    {
        storage::DeviceRepository repo;
        std::vector<storage::DeviceConfig> devices;

        if (repo.loadAllEnabled(devices)) {
            std::cout << "\n[master-server] 设备配置加载成功，共 " << devices.size() << " 台设备" << std::endl;
            std::cout << "------------------------------------------------------------" << std::endl;
            for (const auto& device : devices) {
                std::cout << "  [" << device.deviceCode << "] " << device.deviceName << std::endl
                          << "      站点: " << device.stationName << " | 区域: " << device.areaName << std::endl
                          << "      通信: " << device.ipAddress << ":" << device.port
                          << " (" << device.protocol << " | CA=" << device.commonAddress << ")" << std::endl
                          << "      测点: " << device.points.size() << " 个" << std::endl;
                for (const auto& point : device.points) {
                    std::cout << "        - [" << point.pointCode << "] " << point.pointName
                              << " (IOA=" << point.ioa << ", " << point.pointType << ")";
                    if (point.pointType == "YC" && point.limitLow > 0) {
                        std::cout << " [" << std::fixed << std::setprecision(1)
                                  << point.limitLow << "-" << point.limitHigh << " "
                                  << point.unit << "]";
                    }
                    std::cout << std::endl;
                }
                std::cout << "------------------------------------------------------------" << std::endl;
            }
        } else {
            std::cerr << "[master-server] 设备配置加载失败，请检查" << std::endl;
        }
    }

    /* 迭代2：启动 104；迭代3：启动 UiBroadcaster */
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[master-server] 已停止" << std::endl;
    return 0;
}

void MasterApplication::requestStop()
{
    running_ = false;
}

}  // namespace master
