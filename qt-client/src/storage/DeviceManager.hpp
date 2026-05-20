#pragma once

/**
 * @file   DeviceManager.hpp
 * @brief  Qt 侧 MySQL 设备/测点 CRUD（轻量封装，直连 MySQL）
 * @module qt-client
 */

#include <string>
#include <vector>

#include <mysql.h>

/** @brief 设备简要信息（用于表格展示） */
struct DeviceInfo {
    int id;
    std::string deviceCode;
    std::string deviceName;
    std::string deviceType;
    std::string stationName;
    std::string areaName;
    std::string ipAddress;
    int port;
    std::string protocol;
    int commonAddress;
    int connectTimeoutSec;
    int reconnectIntervalSec;
    bool enabled;
};

/** @brief 测点简要信息 */
struct PointInfo {
    int id;
    int deviceId;
    int ioa;
    std::string pointCode;
    std::string pointName;
    std::string pointType;
    std::string dataType;
    std::string unit;
    double limitHigh;
    double limitLow;
    bool enabled;
};

/**
 * @brief 设备管理器——直接操作 MySQL 数据库
 *
 * 所有变更立即写入 MySQL。变更后需通知主站热加载 (sendReload)。
 */
class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    /** @brief 连接 MySQL */
    bool initialize(const std::string& host, int port,
                    const std::string& user, const std::string& pass,
                    const std::string& db);

    /** @brief 加载所有设备 */
    bool loadAllDevices(std::vector<DeviceInfo>& devices);

    /** @brief 插入设备 */
    bool insertDevice(const DeviceInfo& d);

    /** @brief 更新设备 */
    bool updateDevice(const DeviceInfo& d);

    /** @brief 删除设备及其测点 */
    bool deleteDevice(int deviceId);

    /** @brief 加载某设备的测点 */
    bool loadPoints(int deviceId, std::vector<PointInfo>& points);

    /** @brief 更新测点（主要用于修改限值） */
    bool updatePoint(const PointInfo& p);

private:
    MYSQL* mysql_;
    bool connected_;
};
