/**
 * @file   SimulatorApplication.cpp
 * @brief  设备模拟器：从 MySQL 动态加载设备 → 独立 TCP 端口 + 线程遥测
 * @module device-simulator
 */

#include "app/SimulatorApplication.hpp"

#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <mysql.h>

#include "net/JsonTcpServer.hpp"
#include "scada/json_protocol.hpp"
#include "scada/types.hpp"

namespace simulator {

namespace {
    std::mutex g_randMutex;
}

struct SimulatorApplication::DeviceThread {
    std::string deviceId;
    int port;
    int index;
    std::string label;
    net::JsonTcpServer* server;
    std::thread thread;
};

SimulatorApplication::SimulatorApplication()
    : running_(false)
{
}

SimulatorApplication::~SimulatorApplication()
{
}

/*
 * 从 MySQL 读取 device 表中所有 enabled=1 且 protocol='JSON' 的设备。
 * 失败时回退到硬编码的 3 台设备（兼容无 MySQL 环境）。
 */
int SimulatorApplication::loadDevicesFromDb(std::vector<DeviceThread*>& out)
{
    MYSQL* mysql = mysql_init(NULL);
    if (mysql == NULL) {
        std::cerr << "[simulator] mysql_init 失败，使用硬编码默认设备\n";
        return -1;
    }

    unsigned int timeout = 10;
    mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    if (!mysql_real_connect(mysql, "127.0.0.1", "root", "tanhuang",
                            "scada_demo", 3306, NULL, 0)) {
        std::cerr << "[simulator] MySQL 连接失败: " << mysql_error(mysql)
                  << "，使用硬编码默认设备\n";
        mysql_close(mysql);
        return -1;
    }

    mysql_set_character_set(mysql, "utf8mb4");
    std::cout << "[simulator] MySQL 已连接，加载设备配置...\n";

    const char* sql =
        "SELECT device_code, port FROM device "
        "WHERE enabled=1 AND protocol='JSON' ORDER BY sort_order";

    if (mysql_query(mysql, sql) != 0) {
        std::cerr << "[simulator] 查询失败: " << mysql_error(mysql) << "\n";
        mysql_close(mysql);
        return -1;
    }

    MYSQL_RES* res = mysql_store_result(mysql);
    if (res == NULL) {
        mysql_close(mysql);
        return -1;
    }

    int count = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        DeviceThread* dt = new DeviceThread();
        dt->deviceId = row[0] ? row[0] : "UNKNOWN";
        dt->port = row[1] ? std::stoi(row[1]) : 5001;
        dt->index = count;

        std::ostringstream label;
        label << "[" << dt->deviceId << ":" << dt->port << "]";
        dt->label = label.str();

        dt->server = new net::JsonTcpServer(dt->port);

        out.push_back(dt);
        ++count;
    }

    mysql_free_result(res);
    mysql_close(mysql);

    std::cout << "[simulator] 从 MySQL 加载 " << count << " 台设备\n";
    return count;
}

int SimulatorApplication::run()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    running_ = true;
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    std::cout << "[device-simulator] 启动\n";

    /* 尝试从 MySQL 加载设备列表 */
    std::vector<DeviceThread*> devices;
    int count = loadDevicesFromDb(devices);

    /* MySQL 不可用时回退硬编码默认值 */
    if (count <= 0) {
        std::cout << "[device-simulator] 使用硬编码默认 3 台设备\n"
                  << "  RTU001 → 5001, RTU002 → 5011, RTU003 → 5012\n";
        for (int i = 0; i < 3; ++i) {
            DeviceThread* dt = new DeviceThread();
            dt->index = i;
            if (i == 0) { dt->deviceId = "RTU001"; dt->port = 5001; }
            if (i == 1) { dt->deviceId = "RTU002"; dt->port = 5011; }
            if (i == 2) { dt->deviceId = "RTU003"; dt->port = 5012; }
            std::ostringstream label;
            label << "[" << dt->deviceId << ":" << dt->port << "]";
            dt->label = label.str();
            dt->server = new net::JsonTcpServer(dt->port);
            devices.push_back(dt);
        }
        count = 3;
    }

    for (int i = 0; i < count; ++i) {
        std::cout << "  " << devices[static_cast<std::size_t>(i)]->deviceId
                  << " → 端口 " << devices[static_cast<std::size_t>(i)]->port << "\n";
    }

    /* 启动所有设备的 TCP 服务端 */
    for (int i = 0; i < count; ++i) {
        DeviceThread* dt = devices[static_cast<std::size_t>(i)];
        if (!dt->server->start()) {
            std::cerr << "[device-simulator] " << dt->label
                      << " TCP 服务启动失败\n";
            running_ = false;
            for (int j = 0; j < i; ++j) {
                DeviceThread* d2 = devices[static_cast<std::size_t>(j)];
                if (d2->thread.joinable()) d2->thread.join();
                delete d2->server;
                delete d2;
            }
            return 1;
        }
    }

    /* 启动工作线程 */
    for (int i = 0; i < count; ++i) {
        DeviceThread* dt = devices[static_cast<std::size_t>(i)];
        dt->thread = std::thread(
            &SimulatorApplication::runDeviceThread, this, std::ref(*dt));
    }

    std::cout << "[device-simulator] " << count
              << " 台设备全部启动，等待主站连接...\n";

    /* 等待所有设备线程退出 */
    for (int i = 0; i < count; ++i) {
        DeviceThread* dt = devices[static_cast<std::size_t>(i)];
        if (dt->thread.joinable()) dt->thread.join();
    }

    /* 清理 */
    for (int i = 0; i < count; ++i) {
        DeviceThread* dt = devices[static_cast<std::size_t>(i)];
        dt->server->stop();
        delete dt->server;
        delete dt;
    }

    std::cout << "[device-simulator] 已停止\n";
    return 0;
}

void SimulatorApplication::runDeviceThread(DeviceThread& dt)
{
    scada::device_protocol::JsonProtocol jsonProto;
    scada::Telemetry telem;
    telem.deviceId = dt.deviceId;
    telem.switchState = scada::SwitchState::Closed;

    std::cout << dt.label << " 线程启动\n";

    while (running_) {
        if (!dt.server->hasClient()) {
            dt.server->waitForClient(1000);
            continue;
        }

        /*
         * 检查是否有遥控下行命令（非阻塞，100ms 超时）。
         * 收到 control 帧 → 解码 → 更新开关状态 → 打印日志。
         */
        std::string cmdLine;
        if (dt.server->readLine(cmdLine, 100)) {
            scada::Telemetry ctrl;
            if (scada::device_protocol::JsonProtocol().decodeTelemetry(cmdLine, ctrl)) {
                /* 是 telemetry 帧（正常情况不会收到，但忽略） */
            } else {
                /* 尝试解析为 control 帧 */
                scada::device_protocol::JsonProtocol jp;
                std::string dummy;
                // 简单判断：包含 "control" 关键字
                if (cmdLine.find("\"control\"") != std::string::npos ||
                    cmdLine.find("\"type\":\"control\"") != std::string::npos) {
                    // 解析 switch 值
                    std::size_t pos = cmdLine.find("\"switch\":");
                    if (pos != std::string::npos) {
                        int sw = std::atoi(cmdLine.c_str() + pos + 9);
                        telem.switchState = (sw != 0) ? scada::SwitchState::Closed
                                                       : scada::SwitchState::Open;
                        std::cout << dt.label << " 收到遥控: "
                                  << (sw != 0 ? "合闸" : "分闸") << "\n";
                    }
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(g_randMutex);
            if (telem.switchState == scada::SwitchState::Open) {
                /* 分闸：线路断电，电流归零，电压为残余感应值 */
                telem.voltage = (std::rand() % 50) / 10.0;
                telem.current = 0.0;
            } else {
                /*
                 * 合闸：正常电压，范围 200~250V（超出 215~235 告警阈值以触发告警）。
                 * 约 20% 的概率出现越限值，用于演示告警功能。
                 */
                int rv = std::rand() % 500;  // 0~499
                if (rv < 50) {
                    telem.voltage = 240.0 + (std::rand() % 50) / 10.0;   // 240~245V 越上限
                } else if (rv < 100) {
                    telem.voltage = 210.0 - (std::rand() % 50) / 10.0;   // 205~210V 越下限
                } else {
                    telem.voltage = (220.0 + dt.index * 2.0)
                                  + (std::rand() % 100 - 50) / 10.0;     // 正常 215~235V
                }
                telem.current = (10.0 + dt.index * 2.0 - (dt.index == 2 ? 4.0 : 0.0))
                              + (std::rand() % 100 - 50) / 10.0;
            }
        }
        telem.timestamp = static_cast<std::int64_t>(std::time(NULL));

        std::string frame;
        if (jsonProto.encodeTelemetry(telem, frame)) {
            if (!dt.server->sendLine(frame)) {
                std::cerr << dt.label << " 发送失败，客户端可能已断开\n";
                dt.server->closeClient();
                continue;
            }
            std::cout << dt.label << " >> " << frame;
        }

        for (int i = 0; i < 10 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << dt.label << " 线程退出\n";
}

void SimulatorApplication::requestStop()
{
    running_ = false;
}

}  // namespace simulator
