# SCADA Demo — AI 协作说明

> **详细需求、表结构、每迭代演示步骤** → 见 `docs/`。本文只写 AI 写代码时必须遵守的约定。

## 项目使命

设备中心模型的 SCADA 演示（纯软件模拟）：

```text
device-simulator ──JSON:5001──► master-server ──TCP:5002/5003──► qt-client
        │                              │
        │ (预留 IEC104:2404)           ▼ MySQL
        └── Iec104Protocol 占位
```

- **设备侧当前**：TCP + **单行 JSON**（`JsonProtocol`）。
- **设备侧预留**：`Iec104Protocol` 空实现，业务层只面向 `IDeviceProtocol`。
- 配置与历史以 **MySQL** 为准；Qt 本地 **SQLite** 仅存界面偏好。

## 文档索引

| 文件 | 何时读 |
|------|--------|
| [docs/roadmap.md](docs/roadmap.md) | 当前迭代的交付物 / 演示步骤 / 不做 |
| [docs/requirements.md](docs/requirements.md) | 功能范围、验收 |
| [docs/architecture.md](docs/architecture.md) | 分层、端口、协议接口层 |
| [docs/protocol-device-json.md](docs/protocol-device-json.md) | 设备 JSON 报文格式 |
| [docs/data-model.md](docs/data-model.md) | 表字段、`device.protocol` |
| [docs/coding-standards.md](docs/coding-standards.md) | C++11、注释、Qt .ui |
| [docs/ai-handoff.md](docs/ai-handoff.md) | 给其他 AI 的提示词模板 |

## 协议分层（强制）

```text
业务层（pipeline / storage / push）
        ↓ 只使用 IDeviceProtocol
协议接口层
        ├── JsonProtocol      ← 当前实现
        └── Iec104Protocol    ← 预留，isImplemented()==false
```

- 禁止在业务代码中直接拼设备 JSON 或解析 104 帧。
- 通过 `createProtocolFromName(device.protocol)` 创建实例。
- 104 **勿提前实现**，除非用户明确要求。

## 当前迭代

**迭代 6 完成** → 下一：**迭代 7（告警）**  
见 [roadmap §迭代 5](docs/roadmap.md#迭代-5设备与测点维护)。

## 通信与端口

| 链路 | 协议 | 端口 | 实现 |
|------|------|------|------|
| 模拟器 ↔ 主站 | **JSON 行** | 5001 | `JsonProtocol`，见 protocol-device-json.md |
| 模拟器 ↔ 主站（预留） | IEC 104 | 2404 | `Iec104Protocol` 占位 |
| 主站 → Qt | 文本行 | 5002 | `common/protocol.hpp` `UPDATE\|...` |
| Qt → 主站 | 文本行 | 5003 | `CTRL` / `LOGIN` |

常量：`common/include/scada/config_defaults.hpp`（`kDeviceJsonPort`、`kIec104ReservedPort`）。

### 主站 ↔ Qt 帧（与设备 JSON 分离）

| 方向 | 前缀 | 示例 |
|------|------|------|
| 推送 | `UPDATE` | `UPDATE\|RTU001\|228.5\|12.3\|1\|0` |
| 遥控 | `CTRL` | `CTRL\|RTU001\|1` |
| 登录 | `LOGIN` / `LOGIN_ACK` | `LOGIN\|admin\|123456` |

`TELEM|...` 为早期脚手架文本，**设备侧以 JSON 为准**；主站收到 JSON 后转内存 `Telemetry` 再推 `UPDATE`。

## 技术栈

- **C++11**、CMake 3.16+、Qt **5.15**（Widgets、Network、Charts）
- MySQL 8.x（`scada_demo`）
- 开发 Windows + Qt Creator；主站目标 Linux

## 代码放置约定

| 模块 | 路径 |
|------|------|
| 协议接口 / JSON / 104 占位 | `common/include/scada/device_protocol.hpp`、`json_protocol.*`、`iec104_protocol.*`、`protocol_factory.*` |
| 主站↔Qt 文本协议 | `common/include/scada/protocol.hpp` |
| 主站业务 | `master-server/src/pipeline/` |
| 主站网络 | `master-server/src/net/` |
| 主站存库 | `master-server/src/storage/` |
| 主站推送 | `master-server/src/push/` |
| 模拟器 | `device-simulator/src/app/`、`src/net/` |
| Qt 布局 | `qt-client/ui/*.ui` |
| Qt 页面 | `qt-client/src/pages/` |

## 编码规范

见 [docs/coding-standards.md](docs/coding-standards.md)：C++11、完整注释、Qt Designer。

## 构建与联调

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="D:/soft/Qt/5.15.2/mingw81_64" -G "MinGW Makefiles"
cmake --build build
```

顺序：**device-simulator → master-server → qt-client**。

## 实现进度

| 迭代 | 状态 |
|------|------|
| 0 脚手架 | 完成 |
| 1 MySQL 基座 | 完成 |
| 2 JSON 单设备 | 完成 |
| 3 Qt 实时监控 | 完成 |
| 4 三设备在线 | 完成 |
| 5 设备维护 | 完成 |
| 6 遥控日志 | 完成 |
| 7 告警 | 未开始 |
| 8 曲线登录联调 | 未开始 |

## AI 边界

- 业务层绕过 `IDeviceProtocol` 写死 JSON/104
- 未经要求实现完整 IEC104
- Qt 连接 5001 解析设备 JSON（设备通道只在主站）
- 在 Qt `.cpp` 手写整套主界面（须 `.ui`）
- C++14/17、SQLite 作业务库
- 改 `build/`、`.qtcreator/`、`temp/`
