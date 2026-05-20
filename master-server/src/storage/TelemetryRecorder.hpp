#pragma once

/**
 * @file   TelemetryRecorder.hpp
 * @brief  遥测历史记录器——将 Telemetry 采样写入 MySQL telemetry 表
 * @module master-server
 */

#include "scada/types.hpp"

namespace master {
namespace storage {

/**
 * @brief 遥测持久化：每次遥测写入 3 条记录（电压/电流/开关）
 */
class TelemetryRecorder {
public:
    TelemetryRecorder();

    /**
     * @brief 将一条遥测写入 telemetry 表（3 行：电压 YC / 电流 YC / 开关 YX）
     * @param telem  遥测快照
     */
    void record(const scada::Telemetry& telem);
};

}  // namespace storage
}  // namespace master
