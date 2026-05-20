-- ========================================
-- @file    seed.sql
-- @brief   SCADA 演示项目 MySQL 种子数据（3台设备 + 测点）
-- @module  docs
-- ========================================

USE scada_demo;

-- ========================================
-- 1. 设备配置种子（RTU001-RTU003）
-- ========================================
INSERT IGNORE INTO device (
    device_code, device_name, device_type, station_name, area_name,
    rated_voltage, description, ip_address, port, protocol,
    common_address, link_address, enabled, sort_order
) VALUES
-- RTU001：城南配电站一号开关终端
('RTU001', '城南配电站一号开关终端', '配电终端', '城南配电站', '一区',
 '10kV', '演示设备 - 10kV 线路采集', '127.0.0.1', 5001, 'JSON',
 1, 1, 1, 1),

-- RTU002：城南配电站二号开关终端
('RTU002', '城南配电站二号开关终端', '配电终端', '城南配电站', '一区',
 '10kV', '演示设备 - 10kV 线路采集', '127.0.0.1', 5001, 'JSON',
 2, 2, 1, 2),

-- RTU003：城北环网柜A柜终端
('RTU003', '城北环网柜A柜终端', '环网柜', '城北环网柜', '二区',
 '10kV', '演示设备 - 环网柜采集', '127.0.0.1', 5001, 'JSON',
 3, 3, 1, 3);

-- ========================================
-- 2. 测点定义种子（每个设备3个默认测点：电压、电流、开关）
-- ========================================

-- RTU001 测点
INSERT IGNORE INTO point (
    device_id, ioa, point_code, point_name, point_type, data_type, unit, limit_high, limit_low, enabled
) VALUES
-- 电压 (YC)
((SELECT id FROM device WHERE device_code = 'RTU001'), 1001, 'VOLTAGE', 'A相电压', 'YC', 'FLOAT', 'V', 235.00, 215.00, 1),
-- 电流 (YC)
((SELECT id FROM device WHERE device_code = 'RTU001'), 1002, 'CURRENT', 'A相电流', 'YC', 'FLOAT', 'A', 30.00, 0.00, 1),
-- 开关状态 (YX)
((SELECT id FROM device WHERE device_code = 'RTU001'), 1003, 'SWITCH', '开关状态', 'YX', 'BOOL', '', 1, 0, 1);

-- RTU002 测点
INSERT IGNORE INTO point (
    device_id, ioa, point_code, point_name, point_type, data_type, unit, limit_high, limit_low, enabled
) VALUES
-- 电压 (YC)
((SELECT id FROM device WHERE device_code = 'RTU002'), 2001, 'VOLTAGE', 'A相电压', 'YC', 'FLOAT', 'V', 235.00, 215.00, 1),
-- 电流 (YC)
((SELECT id FROM device WHERE device_code = 'RTU002'), 2002, 'CURRENT', 'A相电流', 'YC', 'FLOAT', 'A', 30.00, 0.00, 1),
-- 开关状态 (YX)
((SELECT id FROM device WHERE device_code = 'RTU002'), 2003, 'SWITCH', '开关状态', 'YX', 'BOOL', '', 1, 0, 1);

-- RTU003 测点
INSERT IGNORE INTO point (
    device_id, ioa, point_code, point_name, point_type, data_type, unit, limit_high, limit_low, enabled
) VALUES
-- 电压 (YC)
((SELECT id FROM device WHERE device_code = 'RTU003'), 3001, 'VOLTAGE', 'A相电压', 'YC', 'FLOAT', 'V', 235.00, 215.00, 1),
-- 电流 (YC)
((SELECT id FROM device WHERE device_code = 'RTU003'), 3002, 'CURRENT', 'A相电流', 'YC', 'FLOAT', 'A', 30.00, 0.00, 1),
-- 开关状态 (YX)
((SELECT id FROM device WHERE device_code = 'RTU003'), 3003, 'SWITCH', '开关状态', 'YX', 'BOOL', '', 1, 0, 1);

-- ========================================
-- 验证数据
-- ========================================
SELECT COUNT(*) as device_count FROM device;
SELECT COUNT(*) as point_count FROM point;
SELECT d.device_code, COUNT(p.id) as point_num
FROM device d
LEFT JOIN point p ON d.id = p.device_id
WHERE d.enabled = 1
GROUP BY d.id, d.device_code
ORDER BY d.sort_order;
