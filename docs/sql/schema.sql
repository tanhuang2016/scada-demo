-- ========================================
-- @file    schema.sql
-- @brief   SCADA 演示项目 MySQL 业务库表结构
-- @module  docs
-- ========================================

-- 创建数据库（如果不存在）
CREATE DATABASE IF NOT EXISTS scada_demo DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE scada_demo;

-- ========================================
-- 设备配置表
-- ========================================
CREATE TABLE IF NOT EXISTS device (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '主键ID',
    device_code VARCHAR(32) NOT NULL UNIQUE COMMENT '设备唯一编码，如 RTU001',
    device_name VARCHAR(128) NOT NULL COMMENT '设备名称',
    device_type VARCHAR(64) NOT NULL COMMENT '设备类型，如 配电终端',
    station_name VARCHAR(128) NOT NULL COMMENT '所属站点',
    area_name VARCHAR(64) NOT NULL COMMENT '所属区域',
    rated_voltage VARCHAR(32) NOT NULL COMMENT '额定电压，如 10kV',
    description TEXT COMMENT '备注说明',
    ip_address VARCHAR(64) NOT NULL COMMENT '通信IP地址',
    port INT NOT NULL COMMENT '通信端口',
    protocol VARCHAR(32) NOT NULL COMMENT '通信协议：JSON（默认）或 IEC104（预留）',
    common_address INT NOT NULL COMMENT 'IEC104 ASDU 公共地址',
    link_address INT NOT NULL COMMENT 'IEC104 链路地址',
    connect_timeout_sec INT NOT NULL DEFAULT 30 COMMENT '连接超时时间（秒）',
    reconnect_interval_sec INT NOT NULL DEFAULT 5 COMMENT '重连间隔（秒）',
    enabled TINYINT(1) NOT NULL DEFAULT 1 COMMENT '是否启用：1=启用，0=停用',
    sort_order INT NOT NULL DEFAULT 0 COMMENT '排序顺序',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_device_code (device_code),
    INDEX idx_enabled (enabled)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='设备配置表';

-- ========================================
-- 测点定义表
-- ========================================
CREATE TABLE IF NOT EXISTS point (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '主键ID',
    device_id INT NOT NULL COMMENT '所属设备ID，关联 device.id',
    ioa INT NOT NULL COMMENT 'IEC104 信息对象地址',
    point_code VARCHAR(32) NOT NULL COMMENT '测点编码，如 VOLTAGE',
    point_name VARCHAR(128) NOT NULL COMMENT '测点名称',
    point_type VARCHAR(32) NOT NULL COMMENT '测点类型：YC=遥测，YX=遥信，YK=遥控',
    data_type VARCHAR(32) NOT NULL COMMENT '数据类型：FLOAT, BOOL',
    unit VARCHAR(32) COMMENT '单位，如 V, A',
    limit_high DECIMAL(10,2) COMMENT '告警上限',
    limit_low DECIMAL(10,2) COMMENT '告警下限',
    enabled TINYINT(1) NOT NULL DEFAULT 1 COMMENT '是否采集：1=启用，0=停用',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_device_id (device_id),
    INDEX idx_ioa (ioa),
    INDEX idx_point_code (point_code),
    FOREIGN KEY (device_id) REFERENCES device(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='测点定义表';

-- ========================================
-- 历史遥测表
-- ========================================
CREATE TABLE IF NOT EXISTS telemetry (
    id BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '主键ID',
    device_id INT NOT NULL COMMENT '所属设备ID',
    point_id INT NOT NULL COMMENT '测点ID',
    device_code VARCHAR(32) NOT NULL COMMENT '设备编码（冗余，方便查询）',
    point_code VARCHAR(32) NOT NULL COMMENT '测点编码（冗余）',
    point_type VARCHAR(32) NOT NULL COMMENT '测点类型',
    value_float DECIMAL(10,2) COMMENT '浮点值（YC用）',
    value_bool TINYINT(1) COMMENT '布尔值（YX用）',
    quality VARCHAR(32) COMMENT '质量标识：GOOD, INVALID',
    timestamp TIMESTAMP NOT NULL COMMENT '采样时间',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '入库时间',
    INDEX idx_device_id (device_id),
    INDEX idx_timestamp (timestamp),
    INDEX idx_device_time (device_id, timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='历史遥测表';

-- ========================================
-- 告警表
-- ========================================
CREATE TABLE IF NOT EXISTS alarm (
    id BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '主键ID',
    device_id INT NOT NULL COMMENT '所属设备ID',
    device_code VARCHAR(32) NOT NULL COMMENT '设备编码',
    alarm_type VARCHAR(64) NOT NULL COMMENT '告警类型：VOLTAGE_HIGH, VOLTAGE_LOW, COMM_LOST',
    alarm_level VARCHAR(32) NOT NULL COMMENT '告警级别：INFO, WARNING, CRITICAL',
    title VARCHAR(256) NOT NULL COMMENT '告警标题',
    description TEXT COMMENT '告警详细描述',
    value_before DECIMAL(10,2) COMMENT '告警前值',
    value_current DECIMAL(10,2) COMMENT '告警当前值',
    limit_high DECIMAL(10,2) COMMENT '当时上限',
    limit_low DECIMAL(10,2) COMMENT '当时下限',
    state VARCHAR(32) NOT NULL COMMENT '状态：ACTIVE, ACKNOWLEDGED, CLEARED',
    occurred_at TIMESTAMP NOT NULL COMMENT '告警发生时间',
    acknowledged_at TIMESTAMP NULL COMMENT '确认时间',
    cleared_at TIMESTAMP NULL COMMENT '清除时间',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    INDEX idx_device_id (device_id),
    INDEX idx_occurred_at (occurred_at),
    INDEX idx_state (state),
    INDEX idx_alarm_type (alarm_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='告警表';

-- ========================================
-- 操作日志表
-- ========================================
CREATE TABLE IF NOT EXISTS operation_log (
    id BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '主键ID',
    device_id INT NOT NULL COMMENT '所属设备ID',
    device_code VARCHAR(32) NOT NULL COMMENT '设备编码',
    operator VARCHAR(64) NOT NULL COMMENT '操作员，如 admin',
    operation_type VARCHAR(64) NOT NULL COMMENT '操作类型：SWITCH_ON, SWITCH_OFF',
    operation_desc VARCHAR(256) NOT NULL COMMENT '操作描述',
    result VARCHAR(32) NOT NULL COMMENT '结果：SUCCESS, FAILURE',
    failure_reason TEXT COMMENT '失败原因',
    point_code VARCHAR(32) COMMENT '测点编码（YK用）',
    ioa INT COMMENT '信息对象地址（YK用）',
    before_value DECIMAL(10,2) COMMENT '操作前值',
    after_value DECIMAL(10,2) COMMENT '操作后值',
    operated_at TIMESTAMP NOT NULL COMMENT '操作时间',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    INDEX idx_device_id (device_id),
    INDEX idx_operated_at (operated_at),
    INDEX idx_operator (operator)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='操作日志表';
