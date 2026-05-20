# 设备侧 JSON 协议（当前默认）

> IEC 60870-5-104 暂不实现，预留 `Iec104Protocol`；设备与主站之间先用 **TCP + 单行 JSON** 模拟现场通信。

## 传输

- 传输层：TCP
- 默认端口：`5001`（`scada::config::kDeviceJsonPort`）
- 分帧：一行一条 JSON，以 `\n` 结尾（便于 `readline` 解析）
- 编码：UTF-8

## 消息类型

| type | 方向 | 说明 |
|------|------|------|
| `telemetry` | 模拟器 → 主站 | 周期遥测 |
| `control` | 主站 → 模拟器 | 遥控 |
| `control_ack` | 模拟器 → 主站 | 遥控返校（迭代 6 完善） |

## telemetry

```json
{"type":"telemetry","device":"RTU001","voltage":228.5,"current":12.3,"switch":1,"ts":1716000000}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| device | string | 设备编码 |
| voltage | number | 伏特 |
| current | number | 安培 |
| switch | int | 1=合闸，0=分闸 |
| ts | int64 | Unix 秒（可选，0 表示未填） |

## control

```json
{"type":"control","device":"RTU001","switch":0}
```

## control_ack（预留）

```json
{"type":"control_ack","device":"RTU001","switch":0,"ok":1,"msg":"ok"}
```

## 与数据库 protocol 字段

`device.protocol` 取值：

- `JSON` — 使用 `JsonProtocol`（当前默认）
- `IEC104` — 使用 `Iec104Protocol`（未实现，主站应打日志并跳过或回退）

## 实现位置

| 组件 | 路径 |
|------|------|
| 接口 | `common/include/scada/device_protocol.hpp` |
| JSON | `common/include/scada/json_protocol.hpp` |
| 104 预留 | `common/include/scada/iec104_protocol.hpp` |
| 工厂 | `common/include/scada/protocol_factory.hpp` |

业务层（主站/模拟器）只依赖 `IDeviceProtocol*`，不直接拼 JSON 字符串。
