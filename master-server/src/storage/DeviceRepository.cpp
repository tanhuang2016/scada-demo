/**
 * @file   DeviceRepository.cpp
 * @brief  设备配置仓库实现（真实 MySQL 连接）
 * @module master-server
 */

#include "storage/DeviceRepository.hpp"
#include "storage/MySQLConnection.hpp"

#include <iostream>
#include <vector>

#include <mysql.h>

namespace master {
namespace storage {

namespace {

void printResultRows(const char* scope, MYSQL_RES* res)
{
    unsigned int fieldCount = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    unsigned long long rowCount = mysql_num_rows(res);

    std::cout << "[master-server] MySQL " << scope
              << " rows: " << rowCount << '\n';

    MYSQL_ROW row;
    unsigned long rowIndex = 0;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(res);
        std::cout << "[master-server] MySQL " << scope
                  << " row[" << rowIndex << "]: ";
        for (unsigned int i = 0; i < fieldCount; ++i) {
            if (i > 0) {
                std::cout << " | ";
            }
            std::cout << fields[i].name << "=";
            if (row[i]) {
                std::cout.write(row[i], lengths[i]);
            } else {
                std::cout << "NULL";
            }
        }
        std::cout << '\n';
        ++rowIndex;
    }

    mysql_data_seek(res, 0);
}

}  // namespace

DeviceRepository::DeviceRepository()
{
}

DeviceRepository::~DeviceRepository()
{
}

bool DeviceRepository::loadAllEnabled(std::vector<DeviceConfig>& devices)
{
    devices.clear();

    bool ok = MySQLConnection::instance().queryMany(
        "SELECT id, device_code, device_name, device_type, station_name, area_name, "
        "rated_voltage, description, ip_address, port, protocol, common_address, "
        "link_address, connect_timeout_sec, reconnect_interval_sec, enabled, sort_order "
        "FROM device WHERE enabled = 1 ORDER BY sort_order",
        [&devices](MYSQL* mysql) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (!res) {
                return false;
            }

            printResultRows("loadAllEnabled", res);

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                DeviceConfig device;
                int idx = 0;
                device.id = row[idx++] ? std::stoi(row[idx - 1]) : 0;
                device.deviceCode = row[idx++] ? row[idx - 1] : "";
                device.deviceName = row[idx++] ? row[idx - 1] : "";
                device.deviceType = row[idx++] ? row[idx - 1] : "";
                device.stationName = row[idx++] ? row[idx - 1] : "";
                device.areaName = row[idx++] ? row[idx - 1] : "";
                device.ratedVoltage = row[idx++] ? row[idx - 1] : "";
                device.description = row[idx++] ? row[idx - 1] : "";
                device.ipAddress = row[idx++] ? row[idx - 1] : "";
                device.port = row[idx++] ? std::stoi(row[idx - 1]) : 0;
                device.protocol = row[idx++] ? row[idx - 1] : "";
                device.commonAddress = row[idx++] ? std::stoi(row[idx - 1]) : 1;
                device.linkAddress = row[idx++] ? std::stoi(row[idx - 1]) : 1;
                device.connectTimeoutSec = row[idx++] ? std::stoi(row[idx - 1]) : 30;
                device.reconnectIntervalSec = row[idx++] ? std::stoi(row[idx - 1]) : 5;
                device.enabled = row[idx++] ? (std::stoi(row[idx - 1]) != 0) : true;
                device.sortOrder = row[idx++] ? std::stoi(row[idx - 1]) : 0;

                // 加载该设备的测点
                devices.push_back(device);
            }

            mysql_free_result(res);
            return true;
        }
    );

    if (!ok) {
        return false;
    }

    for (std::vector<DeviceConfig>::iterator it = devices.begin(); it != devices.end(); ++it) {
        if (!loadPointsForDevice(it->id, it->points)) {
            return false;
        }
    }

    return true;
}

bool DeviceRepository::loadOne(const std::string& deviceCode, DeviceConfig& out)
{
    bool ok = MySQLConnection::instance().queryOne(
        "SELECT id, device_code, device_name, device_type, station_name, area_name, "
        "rated_voltage, description, ip_address, port, protocol, common_address, "
        "link_address, connect_timeout_sec, reconnect_interval_sec, enabled, sort_order "
        "FROM device WHERE device_code = '" + deviceCode + "'",
        [&out](MYSQL* mysql) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (!res) {
                return false;
            }

            printResultRows("loadOne", res);

            MYSQL_ROW row = mysql_fetch_row(res);
            if (!row) {
                mysql_free_result(res);
                return false;
            }

            int idx = 0;
            out.id = row[idx++] ? std::stoi(row[idx - 1]) : 0;
            out.deviceCode = row[idx++] ? row[idx - 1] : "";
            out.deviceName = row[idx++] ? row[idx - 1] : "";
            out.deviceType = row[idx++] ? row[idx - 1] : "";
            out.stationName = row[idx++] ? row[idx - 1] : "";
            out.areaName = row[idx++] ? row[idx - 1] : "";
            out.ratedVoltage = row[idx++] ? row[idx - 1] : "";
            out.description = row[idx++] ? row[idx - 1] : "";
            out.ipAddress = row[idx++] ? row[idx - 1] : "";
            out.port = row[idx++] ? std::stoi(row[idx - 1]) : 0;
            out.protocol = row[idx++] ? row[idx - 1] : "";
            out.commonAddress = row[idx++] ? std::stoi(row[idx - 1]) : 1;
            out.linkAddress = row[idx++] ? std::stoi(row[idx - 1]) : 1;
            out.connectTimeoutSec = row[idx++] ? std::stoi(row[idx - 1]) : 30;
            out.reconnectIntervalSec = row[idx++] ? std::stoi(row[idx - 1]) : 5;
            out.enabled = row[idx++] ? (std::stoi(row[idx - 1]) != 0) : true;
            out.sortOrder = row[idx++] ? std::stoi(row[idx - 1]) : 0;

            // 加载测点
            mysql_free_result(res);
            return true;
        }
    );

    if (!ok) {
        return false;
    }

    return loadPointsForDevice(out.id, out.points);
}

int DeviceRepository::countEnabled()
{
    int count = -1;
    MySQLConnection::instance().queryOne(
        "SELECT COUNT(*) FROM device WHERE enabled = 1",
        [&count](MYSQL* mysql) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (!res) {
                return false;
            }

            printResultRows("countEnabled", res);

            MYSQL_ROW row = mysql_fetch_row(res);
            if (row && row[0]) {
                count = std::stoi(row[0]);
            }

            mysql_free_result(res);
            return true;
        }
    );
    return count;
}

int DeviceRepository::countPoints(int deviceId)
{
    int count = -1;
    MySQLConnection::instance().queryOne(
        "SELECT COUNT(*) FROM point WHERE device_id = " + std::to_string(deviceId) + " AND enabled = 1",
        [&count](MYSQL* mysql) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (!res) {
                return false;
            }

            printResultRows("countPoints", res);

            MYSQL_ROW row = mysql_fetch_row(res);
            if (row && row[0]) {
                count = std::stoi(row[0]);
            }

            mysql_free_result(res);
            return true;
        }
    );
    return count;
}

bool DeviceRepository::loadPointsForDevice(int deviceId, std::vector<PointConfig>& points)
{
    points.clear();

    return MySQLConnection::instance().queryMany(
        "SELECT id, device_id, ioa, point_code, point_name, point_type, data_type, "
        "unit, limit_high, limit_low, enabled "
        "FROM point WHERE device_id = " + std::to_string(deviceId) + " ORDER BY ioa",
        [this, &points](MYSQL* mysql) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (!res) {
                return false;
            }

            printResultRows("loadPointsForDevice", res);

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                PointConfig point;
                int idx = 0;
                point.id = row[idx++] ? std::stoi(row[idx - 1]) : 0;
                point.deviceId = row[idx++] ? std::stoi(row[idx - 1]) : 0;
                point.ioa = row[idx++] ? std::stoi(row[idx - 1]) : 0;
                point.pointCode = row[idx++] ? row[idx - 1] : "";
                point.pointName = row[idx++] ? row[idx - 1] : "";
                point.pointType = row[idx++] ? row[idx - 1] : "";
                point.dataType = row[idx++] ? row[idx - 1] : "";
                point.unit = row[idx++] ? row[idx - 1] : "";
                point.limitHigh = row[idx++] ? std::stod(row[idx - 1]) : 0.0;
                point.limitLow = row[idx++] ? std::stod(row[idx - 1]) : 0.0;
                point.enabled = row[idx++] ? (std::stoi(row[idx - 1]) != 0) : true;

                points.push_back(point);
            }

            mysql_free_result(res);
            return true;
        }
    );
}

}  // namespace storage
}  // namespace master
