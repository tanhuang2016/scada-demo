/**
 * @file   DeviceManager.cpp
 * @brief  Qt 侧 MySQL 设备 CRUD 实现
 * @module qt-client
 */

#include "storage/DeviceManager.hpp"

#include <iostream>
#include <sstream>

DeviceManager::DeviceManager()
    : mysql_(NULL)
    , connected_(false)
{
}

DeviceManager::~DeviceManager()
{
    if (mysql_ != NULL) {
        mysql_close(mysql_);
        mysql_ = NULL;
    }
}

bool DeviceManager::initialize(const std::string& host, int port,
                                const std::string& user, const std::string& pass,
                                const std::string& db)
{
    mysql_ = mysql_init(NULL);
    if (mysql_ == NULL) return false;

    unsigned int timeout = 10;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    my_bool reconnect = 1;
    mysql_options(mysql_, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(mysql_, host.c_str(), user.c_str(), pass.c_str(),
                            db.c_str(), port, NULL, 0)) {
        std::cerr << "[qt-db] MySQL 连接失败: " << mysql_error(mysql_) << "\n";
        mysql_close(mysql_);
        mysql_ = NULL;
        return false;
    }

    mysql_set_character_set(mysql_, "utf8mb4");
    connected_ = true;
    std::cout << "[qt-db] MySQL 已连接\n";
    return true;
}

bool DeviceManager::loadAllDevices(std::vector<DeviceInfo>& devices)
{
    if (!connected_) return false;
    devices.clear();

    if (mysql_query(mysql_, "SELECT id, device_code, device_name, device_type, "
                    "station_name, area_name, ip_address, port, protocol, "
                    "common_address, connect_timeout_sec, reconnect_interval_sec, enabled "
                    "FROM device ORDER BY sort_order") != 0) {
        return false;
    }

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (res == NULL) return false;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        DeviceInfo d;
        int i = 0;
        d.id = row[i++] ? std::stoi(row[i-1]) : 0;
        d.deviceCode = row[i++] ? row[i-1] : "";
        d.deviceName = row[i++] ? row[i-1] : "";
        d.deviceType = row[i++] ? row[i-1] : "";
        d.stationName = row[i++] ? row[i-1] : "";
        d.areaName = row[i++] ? row[i-1] : "";
        d.ipAddress = row[i++] ? row[i-1] : "";
        d.port = row[i++] ? std::stoi(row[i-1]) : 0;
        d.protocol = row[i++] ? row[i-1] : "";
        d.commonAddress = row[i++] ? std::stoi(row[i-1]) : 1;
        d.connectTimeoutSec = row[i++] ? std::stoi(row[i-1]) : 30;
        d.reconnectIntervalSec = row[i++] ? std::stoi(row[i-1]) : 5;
        d.enabled = row[i] ? (std::stoi(row[i]) != 0) : true;
        devices.push_back(d);
    }
    mysql_free_result(res);
    return true;
}

bool DeviceManager::insertDevice(const DeviceInfo& d)
{
    if (!connected_) return false;
    std::ostringstream sql;
    sql << "INSERT INTO device (device_code, device_name, device_type, "
        << "station_name, area_name, rated_voltage, ip_address, port, protocol, "
        << "common_address, link_address, connect_timeout_sec, reconnect_interval_sec, "
        << "enabled, sort_order) VALUES ("
        << "'" << d.deviceCode << "',"
        << "'" << d.deviceName << "',"
        << "'" << d.deviceType << "',"
        << "'" << d.stationName << "',"
        << "'" << d.areaName << "',"
        << "'-',"
        << "'" << d.ipAddress << "',"
        << d.port << ","
        << "'" << d.protocol << "',"
        << d.commonAddress << ","
        << "1,"
        << d.connectTimeoutSec << ","
        << d.reconnectIntervalSec << ","
        << (d.enabled ? 1 : 0) << ","
        << "99)";
    return mysql_query(mysql_, sql.str().c_str()) == 0;
}

bool DeviceManager::updateDevice(const DeviceInfo& d)
{
    if (!connected_) return false;
    std::ostringstream sql;
    sql << "UPDATE device SET "
        << "device_code='" << d.deviceCode << "',"
        << "device_name='" << d.deviceName << "',"
        << "device_type='" << d.deviceType << "',"
        << "station_name='" << d.stationName << "',"
        << "area_name='" << d.areaName << "',"
        << "ip_address='" << d.ipAddress << "',"
        << "port=" << d.port << ","
        << "protocol='" << d.protocol << "',"
        << "common_address=" << d.commonAddress << ","
        << "connect_timeout_sec=" << d.connectTimeoutSec << ","
        << "reconnect_interval_sec=" << d.reconnectIntervalSec << ","
        << "enabled=" << (d.enabled ? 1 : 0)
        << " WHERE id=" << d.id;
    return mysql_query(mysql_, sql.str().c_str()) == 0;
}

bool DeviceManager::deleteDevice(int deviceId)
{
    if (!connected_) return false;
    /* 先删测点 */
    mysql_query(mysql_, ("DELETE FROM point WHERE device_id=" + std::to_string(deviceId)).c_str());
    std::ostringstream sql;
    sql << "DELETE FROM device WHERE id=" << deviceId;
    return mysql_query(mysql_, sql.str().c_str()) == 0;
}

bool DeviceManager::loadPoints(int deviceId, std::vector<PointInfo>& points)
{
    if (!connected_) return false;
    points.clear();

    std::string sql = "SELECT id, device_id, ioa, point_code, point_name, "
                      "point_type, data_type, unit, limit_high, limit_low, enabled "
                      "FROM point WHERE device_id=" + std::to_string(deviceId) +
                      " ORDER BY ioa";
    if (mysql_query(mysql_, sql.c_str()) != 0) return false;

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (res == NULL) return false;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        PointInfo p;
        int i = 0;
        p.id = row[i++] ? std::stoi(row[i-1]) : 0;
        p.deviceId = row[i++] ? std::stoi(row[i-1]) : 0;
        p.ioa = row[i++] ? std::stoi(row[i-1]) : 0;
        p.pointCode = row[i++] ? row[i-1] : "";
        p.pointName = row[i++] ? row[i-1] : "";
        p.pointType = row[i++] ? row[i-1] : "";
        p.dataType = row[i++] ? row[i-1] : "";
        p.unit = row[i++] ? row[i-1] : "";
        p.limitHigh = row[i++] ? std::stod(row[i-1]) : 0.0;
        p.limitLow = row[i++] ? std::stod(row[i-1]) : 0.0;
        p.enabled = row[i] ? (std::stoi(row[i]) != 0) : true;
        points.push_back(p);
    }
    mysql_free_result(res);
    return true;
}

bool DeviceManager::updatePoint(const PointInfo& p)
{
    if (!connected_) return false;
    std::ostringstream sql;
    sql << "UPDATE point SET "
        << "point_name='" << p.pointName << "',"
        << "limit_high=" << p.limitHigh << ","
        << "limit_low=" << p.limitLow << ","
        << "enabled=" << (p.enabled ? 1 : 0)
        << " WHERE id=" << p.id;
    return mysql_query(mysql_, sql.str().c_str()) == 0;
}

int DeviceManager::insertPoint(int deviceId, const PointInfo& p)
{
    if (!connected_) return -1;
    std::ostringstream sql;
    sql << "INSERT INTO point (device_id, ioa, point_code, point_name, "
        << "point_type, data_type, unit, limit_high, limit_low, enabled) VALUES ("
        << deviceId << ","
        << p.ioa << ","
        << "'" << p.pointCode << "',"
        << "'" << p.pointName << "',"
        << "'" << p.pointType << "',"
        << "'" << p.dataType << "',"
        << "'" << p.unit << "',"
        << p.limitHigh << ","
        << p.limitLow << ","
        << (p.enabled ? 1 : 0) << ")";
    if (mysql_query(mysql_, sql.str().c_str()) != 0) return -1;
    return static_cast<int>(mysql_insert_id(mysql_));
}

bool DeviceManager::deletePoint(int pointId)
{
    if (!connected_) return false;
    std::ostringstream sql;
    sql << "DELETE FROM point WHERE id=" << pointId;
    return mysql_query(mysql_, sql.str().c_str()) == 0;
}
