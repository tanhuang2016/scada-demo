/**
 * @file   DeviceRepository.hpp
 * @brief  设备与测点配置数据访问对象（Repository 模式）
 * @module master-server
 */

#pragma once

#include <string>
#include <vector>

#include <mysql.h>

namespace master {
namespace storage {

/**
 * @brief 测点配置结构
 */
struct PointConfig {
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

    PointConfig()
        : id(0)
        , deviceId(0)
        , ioa(0)
        , limitHigh(0.0)
        , limitLow(0.0)
        , enabled(true)
    {
    }
};

/**
 * @brief 设备配置结构（包含测点列表）
 */
struct DeviceConfig {
    int id;
    std::string deviceCode;
    std::string deviceName;
    std::string deviceType;
    std::string stationName;
    std::string areaName;
    std::string ratedVoltage;
    std::string description;
    std::string ipAddress;
    int port;
    std::string protocol;
    int commonAddress;
    int linkAddress;
    int connectTimeoutSec;
    int reconnectIntervalSec;
    bool enabled;
    int sortOrder;
    std::vector<PointConfig> points;

    DeviceConfig()
        : id(0)
        , port(0)
        , commonAddress(1)
        , linkAddress(1)
        , connectTimeoutSec(30)
        , reconnectIntervalSec(5)
        , enabled(true)
        , sortOrder(0)
    {
    }
};

/**
 * @brief 设备配置仓库（从 MySQL 读取 device 和 point 表）
 *
 * 主要功能：
 * - 加载全部启用的设备列表
 * - 根据设备编码加载单台设备及其测点
 * - 统计数据
 */
class DeviceRepository {
public:
    DeviceRepository();
    ~DeviceRepository();

    /**
     * @brief 加载全部启用的设备及其测点
     * @param devices 输出参数，加载成功的设备列表
     * @return 加载是否成功
     */
    bool loadAllEnabled(std::vector<DeviceConfig>& devices);

    /**
     * @brief 加载单台设备及其测点
     * @param deviceCode 设备编码，如 RTU001
     * @param out 输出参数，设备配置
     * @return 是否找到并成功加载
     */
    bool loadOne(const std::string& deviceCode, DeviceConfig& out);

    /**
     * @brief 获取启用的设备数量
     * @return 设备数量，-1 表示查询失败
     */
    int countEnabled();

    /**
     * @brief 获取设备的测点数量
     * @param deviceId 设备 ID
     * @return 测点数量，-1 表示查询失败
     */
    int countPoints(int deviceId);

private:
    /** 辅助函数：从 MYSQL_ROW 填充 DeviceConfig（不含 points） */
    bool fillDeviceFromRow(MYSQL_RES* res, MYSQL_ROW row, DeviceConfig& out);

    /** 辅助函数：从 MYSQL_ROW 填充 PointConfig */
    bool fillPointFromRow(MYSQL_RES* res, MYSQL_ROW row, PointConfig& out);

    /** 辅助函数：加载指定设备的所有测点 */
    bool loadPointsForDevice(int deviceId, std::vector<PointConfig>& points);
};

}  // namespace storage
}  // namespace master
