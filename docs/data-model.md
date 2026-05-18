# 数据模型与表设计摘要

> 业务库：**MySQL**。Qt 本地偏好：**SQLite**（`qt-client/data/client_config.db`），见 [requirements.md](requirements.md#qt-本地-sqlite)。

## 设备（device）

### 配置字段

| 字段 | 说明 | 示例 |
|------|------|------|
| id | 主键 | 1 |
| device_code | 唯一编码 | RTU001 |
| device_name | 名称 | 城南配电站一号开关终端 |
| device_type | 类型 | 配电终端 |
| station_name | 所属站点 | 城南配电站 |
| area_name | 所属区域 | 一区 |
| rated_voltage | 额定电压 | 10kV |
| description | 备注 | 演示设备 |
| ip_address | 通信 IP | 127.0.0.1 |
| port | 端口 | 2404 |
| protocol | 协议 | IEC104 |
| common_address | ASDU 公共地址 | 1 |
| link_address | 链路地址 | 1 |
| connect_timeout_sec | 连接超时 | 30 |
| reconnect_interval_sec | 重连间隔 | 5 |
| enabled | 是否启用 | 1 |
| sort_order | 排序 | 1 |
| created_at / updated_at | 时间戳 | 自动 |

### 运行状态（内存，不入库或可选快照表）

| 字段 | 说明 |
|------|------|
| online | 在线/离线 |
| last_comm_time | 最后通信时间 |
| voltage / current | 当前遥测 |
| switch_state | 合闸/分闸 |
| alarm_state | 正常/告警 |

## 测点（point）

| 字段 | 说明 | 示例 |
|------|------|------|
| id | 主键 | 1 |
| device_id | 所属设备 | 1 |
| ioa | IEC104 信息对象地址 | 1001 |
| point_code | 测点编码 | VOLTAGE |
| point_name | 名称 | A相电压 |
| point_type | YC/YX/YK | 遥测 |
| data_type | FLOAT/BOOL | FLOAT |
| unit | 单位 | V |
| limit_high / limit_low | 告警上下限 | 235 / 215 |
| enabled | 是否采集 | 1 |

默认测点（每台设备）：电压、电流、开关状态。

## MySQL 业务表

| 表名 | 用途 |
|------|------|
| `device` | 设备配置 |
| `point` | 测点定义 |
| `telemetry` | 历史采样（电压、电流等） |
| `alarm` | 越限、通信中断等 |
| `operation_log` | 遥控合闸/分闸记录 |
| `system_log` | 可选，主站运行日志 |

## 种子数据（演示默认 3 台设备）

| 编码 | 名称 |
|------|------|
| RTU001 | 城南配电站一号开关终端 |
| RTU002 | 城南配电站二号开关终端 |
| RTU003 | 城北环网柜A柜终端 |

模拟量范围：电压 **215–235 V**，电流 **0–30 A**。

## Qt 本地 SQLite（非业务）

| 表 | 用途 |
|----|------|
| `user_preference` | 窗口大小、主题、最近设备等键值 |
| `recent_query` | 历史曲线最近查询时间范围 |

登录「记住用户名 / 自动登录」存于此库，**不建用户表**。
