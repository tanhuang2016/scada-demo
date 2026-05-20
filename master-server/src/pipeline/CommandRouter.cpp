/**
 * @file   CommandRouter.cpp
 * @brief  遥控命令路由实现
 * @module master-server
 */

#include "pipeline/CommandRouter.hpp"

#include <iostream>
#include <sstream>
#include <ctime>

#include "net/DeviceConnector.hpp"
#include "storage/MySQLConnection.hpp"

namespace master {
namespace pipeline {

CommandRouter::CommandRouter()
{
}

CommandRouter::~CommandRouter()
{
}

void CommandRouter::registerDevice(const storage::DeviceConfig& cfg,
                                    net::DeviceConnector* connector,
                                    scada::device_protocol::IDeviceProtocol* protocol)
{
    std::lock_guard<std::mutex> lock(mutex_);
    DeviceEntry entry;
    entry.config = cfg;
    entry.connector = connector;
    entry.protocol = protocol;
    devices_.push_back(entry);
}

void CommandRouter::unregisterDevice(const std::string& deviceCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (std::vector<DeviceEntry>::iterator it = devices_.begin();
         it != devices_.end(); ++it) {
        if (it->config.deviceCode == deviceCode) {
            devices_.erase(it);
            return;
        }
    }
}

std::string CommandRouter::handleCtrl(const std::string& deviceCode, int switchVal)
{
    /* 查找目标设备 */
    DeviceEntry* entry = NULL;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (std::vector<DeviceEntry>::iterator it = devices_.begin();
             it != devices_.end(); ++it) {
            if (it->config.deviceCode == deviceCode) {
                entry = &(*it);
                break;
            }
        }
    }

    if (entry == NULL) {
        logOperation(deviceCode, 0, (switchVal != 0 ? "SWITCH_ON" : "SWITCH_OFF"),
                     "FAILURE", -1, -1, "设备未在线");
        return "FAILURE";
    }

    if (!entry->connector->isConnected()) {
        logOperation(deviceCode, entry->config.id,
                     (switchVal != 0 ? "SWITCH_ON" : "SWITCH_OFF"),
                     "FAILURE", -1, -1, "设备未连接");
        return "FAILURE";
    }

    /* 通过 IDeviceProtocol 编码遥控帧（不直接拼 JSON/104） */
    scada::SwitchState target = (switchVal != 0) ? scada::SwitchState::Closed
                                                  : scada::SwitchState::Open;
    std::string frame;
    if (!entry->protocol->encodeControl(deviceCode, target, frame)) {
        logOperation(deviceCode, entry->config.id,
                     (switchVal != 0 ? "SWITCH_ON" : "SWITCH_OFF"),
                     "FAILURE", -1, -1, "协议编码失败");
        return "FAILURE";
    }

    /* 发送遥控帧到设备 */
    if (!entry->connector->sendLine(frame)) {
        logOperation(deviceCode, entry->config.id,
                     (switchVal != 0 ? "SWITCH_ON" : "SWITCH_OFF"),
                     "FAILURE", -1, -1, "发送失败");
        return "FAILURE";
    }

    /* 记录操作日志（乐观记录——认为设备会执行） */
    logOperation(deviceCode, entry->config.id,
                 (switchVal != 0 ? "SWITCH_ON" : "SWITCH_OFF"),
                 "SUCCESS", -1, switchVal, "");

    std::cout << "[command] " << deviceCode
              << (switchVal != 0 ? " 合闸" : " 分闸") << " 成功\n";
    return "SUCCESS";
}

/*
 * 写入 operation_log 到 MySQL。
 * 使用 MySQLConnection 单例（已在 MasterApplication::run 中初始化）。
 */
bool CommandRouter::logOperation(const std::string& deviceCode, int deviceId,
                                  const std::string& opType, const std::string& result,
                                  int beforeVal, int afterVal, const std::string& reason)
{
    std::ostringstream sql;
    sql << "INSERT INTO operation_log "
        << "(device_id, device_code, operator, operation_type, operation_desc, "
        << "result, failure_reason, operated_at) VALUES ("
        << deviceId << ","
        << "'" << deviceCode << "',"
        << "'admin',"
        << "'" << opType << "',"
        << "'" << (opType == "SWITCH_ON" ? "合闸操作" : "分闸操作") << "',"
        << "'" << result << "',"
        << "'" << reason << "',"
        << "NOW())";

    int rows = storage::MySQLConnection::instance().execute(sql.str());
    return rows >= 0;
}

}  // namespace pipeline
}  // namespace master
