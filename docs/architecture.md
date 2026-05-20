# 系统架构

## 总体结构

```text
device-simulator  ──JSON/TCP:5001──►  master-server  ──文本行:5002/5003──►  qt-client
     (子站)              (主站)                         (监控与配置)
                              │
                              ▼
                           MySQL
```

- **设备侧（当前）**：TCP + **单行 JSON**（端口 **5001**），由 `JsonProtocol` 实现。
- **设备侧（预留）**：IEC 60870-5-104（端口 **2404**），由 `Iec104Protocol` 占位，后续可插拔。
- **主站侧**：读 MySQL → 经**协议接口层**采集 → 业务层（告警/入库）→ 推送 Qt。
- **Qt 侧**：五类页面 + 登录；本地 SQLite 仅存界面偏好。

## 分层（主站 / 模拟器）

```text
┌─────────────────────────────────────┐
│  业务层 pipeline / storage / push   │  告警、遥控路由、MySQL、推 UI
├─────────────────────────────────────┤
│  协议接口层  IDeviceProtocol         │
│    ├── JsonProtocol      （当前实现） │
│    └── Iec104Protocol    （预留空实现）│
├─────────────────────────────────────┤
│  传输层  TCP accept / connect         │
└─────────────────────────────────────┘
```

业务代码**只依赖** `IDeviceProtocol`，通过 `createProtocolFromName(device.protocol)` 选择实现。详见 [protocol-device-json.md](protocol-device-json.md)。

## 三个程序

| 程序 | 职责 |
|------|------|
| `device-simulator` | 监听 5001；周期 JSON 上送；响应 JSON 遥控 |
| `master-server` | 加载配置；按协议字段选 JSON/104；告警；MySQL；推送 Qt |
| `qt-client` | 设备管理、监控、告警、曲线、日志、登录 |

## 端口

| 端口 | 用途 |
|------|------|
| 5001 | 设备 JSON（当前） |
| 2404 | IEC104 预留，未启用 |
| 5002 | 主站 → Qt 推送 |
| 5003 | Qt → 主站 控制/登录 |

## 主站 ↔ Qt

与设备 JSON **分离**，沿用 `common` 文本行（`UPDATE`/`CTRL`/`LOGIN`），避免 UI 进程解析设备协议。

## 代码位置

| 层级 | 路径 |
|------|------|
| 协议接口 | `common/include/scada/device_protocol.hpp` |
| JSON | `common/include/scada/json_protocol.hpp` |
| 104 预留 | `common/include/scada/iec104_protocol.hpp` |
| 工厂 | `common/include/scada/protocol_factory.hpp` |
| 主站业务 | `master-server/src/pipeline/`、`storage/`、`push/` |
| UI 文本协议 | `common/include/scada/protocol.hpp` |

## 未来补充 IEC104

1. 实现 `Iec104Protocol` 各方法（或内部持有 lib60870 会话）。
2. `device.protocol = IEC104` 且 `port = 2404` 的种子数据。
3. 业务层无改动（仍走 `IDeviceProtocol`）。

不在未明确要求时实现 104 细节。
