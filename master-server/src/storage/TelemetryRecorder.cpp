/**
 * @file   TelemetryRecorder.cpp
 * @brief  遥测历史记录器实现
 * @module master-server
 */

#include "storage/TelemetryRecorder.hpp"

#include <sstream>
#include <ctime>

#include "storage/MySQLConnection.hpp"

namespace master {
namespace storage {

TelemetryRecorder::TelemetryRecorder()
{
}

/*
 * 一次遥测写入 3 条记录（每个测点一条）：
 *   VOLTAGE  → value_float = telemetry.voltage
 *   CURRENT  → value_float = telemetry.current
 *   SWITCH   → value_bool = (switchState == Closed)
 *
 * timestamp 字段使用 telemetry.timestamp 的 Unix 秒。
 */
void TelemetryRecorder::record(const scada::Telemetry& telem)
{
    const std::string& dev = telem.deviceId;

    /* 将 Unix 秒转为 MySQL DATETIME 字符串 */
    std::time_t ts = static_cast<std::time_t>(telem.timestamp);
    char timeBuf[32] = {};
#ifdef _WIN32
    struct tm tmBuf;
    localtime_s(&tmBuf, &ts);
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmBuf);
#else
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&ts));
#endif

    /* 电压 */
    {
        std::ostringstream sql;
        sql << "INSERT INTO telemetry (device_id, point_id, device_code, "
            << "point_code, point_type, value_float, quality, timestamp) VALUES ("
            << "0,0,'" << dev << "','VOLTAGE','YC',"
            << telem.voltage << ",'GOOD','" << timeBuf << "')";
        storage::MySQLConnection::instance().execute(sql.str());
    }

    /* 电流 */
    {
        std::ostringstream sql;
        sql << "INSERT INTO telemetry (device_id, point_id, device_code, "
            << "point_code, point_type, value_float, quality, timestamp) VALUES ("
            << "0,0,'" << dev << "','CURRENT','YC',"
            << telem.current << ",'GOOD','" << timeBuf << "')";
        storage::MySQLConnection::instance().execute(sql.str());
    }

    /* 开关状态 */
    {
        std::ostringstream sql;
        sql << "INSERT INTO telemetry (device_id, point_id, device_code, "
            << "point_code, point_type, value_bool, quality, timestamp) VALUES ("
            << "0,0,'" << dev << "','SWITCH','YX',"
            << (telem.switchState == scada::SwitchState::Closed ? 1 : 0)
            << ",'GOOD','" << timeBuf << "')";
        storage::MySQLConnection::instance().execute(sql.str());
    }
}

}  // namespace storage
}  // namespace master
