/**
 * @file   SimulatorApplication.cpp
 * @brief  设备模拟器实现：TCP 5001 监听 + 周期 JSON 遥测上送
 *
 * 当前模拟一台设备（RTU001），约每秒发送一条遥测。
 *
 * 模拟数据策略：
 *   - 电压：220.0 ± 10V（210~230V），rand() 均匀分布
 *   - 电流：10.0 ± 5A（5~15A）
 *   - 开关：固定合闸（1），迭代6 支持遥控后可变为分闸
 *   - 时间戳：Unix 秒（来自 std::time）
 *
 * 设备标识码硬编码为 "RTU001"，与 MySQL seed.sql 一致。
 * 多设备模拟在迭代4实现。
 *
 * 主循环结构：
 *   1. 等待主站连接（select 1s 超时 → 检查 running_）
 *   2. 生成遥测 → encodeTelemetry（通过 JsonProtocol 而非手动拼 JSON）
 *   3. sendLine → 失败则断开客户端 → 回到步骤1
 *   4. 等待 1 秒（分片为 10×100ms，及时响应 requestStop）
 *
 * @module device-simulator
 */

#include "app/SimulatorApplication.hpp"

#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "net/JsonTcpServer.hpp"
#include "scada/config_defaults.hpp"
#include "scada/json_protocol.hpp"
#include "scada/types.hpp"

namespace simulator {

SimulatorApplication::SimulatorApplication()
    : running_(false)
{
}

SimulatorApplication::~SimulatorApplication()
{
}

int SimulatorApplication::run()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    running_ = true;

    /*
     * 用当前时间播种随机数，使每次启动的遥测序列不同。
     * 使用 std::rand 而非 C++11 <random>：
     *   演示项目只需要简单波动，rand() 精度足够且代码量小。
     */
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    std::cout << "[device-simulator] 启动\n"
              << "  JSON 设备端口: " << scada::config::kDeviceJsonPort << "\n"
              << "  (IEC104 预留端口: " << scada::config::kIec104ReservedPort << ")\n";

    /* 启动 TCP 服务端 */
    net::JsonTcpServer server(scada::config::kDeviceJsonPort);
    if (!server.start()) {
        std::cerr << "[device-simulator] TCP 服务启动失败\n";
        return 1;
    }

    /*
     * 准备模拟遥测模板。
     * 通过 JsonProtocol::encodeTelemetry 编码（遵守分层架构）。
     * 业务代码不直接拼 JSON 字符串。
     */
    scada::device_protocol::JsonProtocol jsonProto;
    scada::Telemetry telem;
    telem.deviceId = "RTU001";                          // 与 MySQL seed 数据保持一致
    telem.switchState = scada::SwitchState::Closed;      // 初始合闸

    std::cout << "[device-simulator] 等待主站连接...\n";

    while (running_) {
        /* 等待主站连接（1 秒超时，允许及时退出） */
        if (!server.hasClient()) {
            server.waitForClient(1000);
            continue;
        }

        /*
         * 生成带随机波动的模拟遥测。
         * 电压范围 210~230V（220V ± 10V），在告警阈值 215~235V 范围内。
         * 偶尔会低于 215V，触发后续迭代的越下限告警验证。
         */
        telem.voltage = 220.0 + (std::rand() % 200 - 100) / 10.0;   // 210~230V
        telem.current = 10.0 + (std::rand() % 100 - 50) / 10.0;     // 5~15A
        telem.timestamp = static_cast<std::int64_t>(std::time(NULL));

        /* 通过 JsonProtocol 编码，业务层不手动拼 JSON */
        std::string frame;
        if (jsonProto.encodeTelemetry(telem, frame)) {
            if (!server.sendLine(frame)) {
                /*
                 * 发送失败 — 通常是主站断开连接。
                 * 关闭客户端端，回到等待连接状态。
                 * 主站侧会通过 DeviceConnector::readLine 的三层策略检测断连。
                 */
                std::cerr << "[device-simulator] 发送失败，客户端可能已断开\n";
                server.closeClient();
                std::cout << "[device-simulator] 等待主站连接...\n";
                continue;
            }
            std::cout << "[device-simulator] >> " << frame;
        }

        /*
         * 1 秒间隔上送。
         * 分片 sleep 为 10×100ms，每片检查 running_。
         * 这样用户按 Ctrl+C 后最多 100ms 内退出，而非卡 1 秒。
         */
        for (int i = 0; i < 10 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    server.stop();
    std::cout << "[device-simulator] 已停止\n";
    return 0;
}

void SimulatorApplication::requestStop()
{
    running_ = false;
}

}  // namespace simulator
