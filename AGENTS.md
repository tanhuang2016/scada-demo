# SCADA Demo — AI 协作说明

## 项目使命

极简电力 SCADA 演示系统（纯软件模拟）：`device-simulator` → `master-server` → `qt-client`。用于学习 C++/Qt/Linux 工业开发及面试演示。当前为**脚手架阶段**，业务逻辑按 8 周计划逐步补齐。

## 技术栈

- C++17、CMake 3.16+
- Qt **5.15**（Widgets、Network；历史曲线后续用 Charts）
- SQLite（主站存库，第 4 周起实现，脚手架尚未链接）
- 主开发环境：**Windows** + Qt Creator；主站目标可迁 Linux

## 仓库结构

```
scada-demo/
├── CMakeLists.txt
├── AGENTS.md
├── README.md
├── common/              # 静态库：协议、类型、端口常量
├── master-server/       # 主站后台，启动类 MasterApplication
├── device-simulator/    # 设备模拟器，启动类 SimulatorApplication
├── qt-client/           # Qt 监控界面，MainWindow
└── docs/
```

## 数据流与端口

| 端口 | 方向 |
|------|------|
| 5001 | device-simulator → master-server |
| 5002 | master-server → qt-client（推送） |
| 5003 | qt-client → master-server（遥控） |

**文本协议（单行，`|` 分隔）**

| 类型 | 示例 |
|------|------|
| 遥测上报 | `TELEM\|dev01\|228.5\|12.3\|1\|1716000000` |
| UI 推送 | `UPDATE\|dev01\|228.5\|12.3\|1\|0`（末字段 alarm） |
| 遥控 | `CTRL\|dev01\|1` |

电压告警：不在 [215, 235] V。电流演示范围 0~30A，暂不告警。

## 编码规范

- 优先 `std::unique_ptr` / `std::shared_ptr`，避免裸 `new`/`delete`
- 网络与 Qt UI 分线程；跨线程用 Qt 信号槽或 `QMetaObject::invokeMethod`
- 公共协议/类型只放在 `common/include/scada/`，三端禁止各自复制
- 应用入口：`main.cpp` 仅负责信号与启动；逻辑放在 `*Application` 或 `MainWindow`

## 构建

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64"
cmake --build build
```

可执行文件输出：`build/bin/`（或 kit 对应目录）。

启动顺序（联调）：`master-server` → `device-simulator` → `qt-client`。

## 实现进度（脚手架）

- [x] 目录与 CMake 多目标
- [x] `common` 协议编解码桩
- [x] 三端启动类占位
- [ ] TCP 通信、多线程收包
- [ ] SQLite 持久化
- [ ] Qt 四遥界面、曲线、告警动画
- [ ] 前后端联调脚本

## AI 边界（勿擅自做）

- 不改为 JSON/gRPC/微服务，除非用户明确要求
- 不引入真实 IEC 104/Modbus，除非明确要求
- 不修改 `build/`、`.qtcreator/`
- 不扩展遥调、真实硬件驱动
- 除 `README.md`/`docs/` 外不批量新增文档

## 8 周代码落点

| 周次 | 目录 |
|------|------|
| 3 | `master-server/src/net/`, `device-simulator/`, `common` 协议 |
| 4 | `master-server/src/storage/`, `push/` |
| 5 | `master-server/src/pipeline/`, 日志 |
| 6-7 | `qt-client/src/widgets/`, `net/` |
| 8 | `scripts/` 联调 |
