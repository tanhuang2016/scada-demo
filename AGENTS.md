# SCADA Demo — AI 协作说明

> **详细需求、表结构、每迭代演示步骤** → 见 `docs/`。本文只写 AI 写代码时必须遵守的约定。

## 项目使命

设备中心模型的 SCADA 演示（纯软件模拟）：

```text
device-simulator ──IEC104:2404──► master-server ──TCP:5002/5003──► qt-client
                                      │
                                      ▼ MySQL（业务）
```

- 现场协议只在 **主站 ↔ 模拟器**；Qt **不**实现 104。
- 配置与历史以 **MySQL** 为准；Qt 本地 **SQLite** 仅存界面偏好。

## 文档索引

| 文件 | 何时读 |
|------|--------|
| [docs/roadmap.md](docs/roadmap.md) | 开工前：当前迭代的交付物 / 演示步骤 / 不做 |
| [docs/requirements.md](docs/requirements.md) | 功能范围、验收、终态演示 |
| [docs/data-model.md](docs/data-model.md) | 表字段、种子设备 RTU001–003 |
| [docs/architecture.md](docs/architecture.md) | 三程序职责、端口 |
| [docs/coding-standards.md](docs/coding-standards.md) | **C++11、注释、Qt .ui 规范（必读）** |
| [docs/ai-handoff.md](docs/ai-handoff.md) | 给其他 AI 工具的提示词模板 |

## 当前迭代

**迭代 0 完成** → 下一：**迭代 1（MySQL 基座）**  
详见 [roadmap §迭代 1](docs/roadmap.md#迭代-1数据与配置基座)。

完成某迭代后：更新下文「实现进度」表，并在 `roadmap.md` 勾选完成标准。

## 通信与端口

| 链路 | 协议 | 端口 | 实现位置 |
|------|------|------|----------|
| 模拟器 → 主站 | IEC 60870-5-104 | 2404 | `device-simulator/`, `master-server/src/net/` |
| 主站 → Qt 推送 | 文本行（见下） | 5002 | `master-server/src/push/`, `qt-client/src/net/` |
| Qt → 主站 控制/登录 | 文本行 | 5003 | `qt-client/src/net/`, `master-server/` 命令处理 |

常量：`common/include/scada/config_defaults.hpp`。

### 主站 ↔ Qt 帧格式（`common` 编解码，`|` 分隔）

| 方向 | 前缀 | 示例 |
|------|------|------|
| 推送实时 | `UPDATE` | `UPDATE\|RTU001\|228.5\|12.3\|1\|0`（末位 alarm: 0/1） |
| 遥控 | `CTRL` | `CTRL\|RTU001\|1`（1=合闸 0=分闸） |
| 登录请求 | `LOGIN` | `LOGIN\|admin\|123456` |
| 登录响应 | `LOGIN_ACK` | `LOGIN_ACK\|1\|ok`（1=成功） |

设备侧 104 与上述帧**分离**；`TELEM|...` 仅为脚手架桩，迭代 2 后设备数据经 104 解析再转为 `UPDATE` 推送。

### 告警（主站判定）

- 电压 ∉ [215, 235] V（或以测点 `limit_low/high` 为准）
- 通信中断：超时未收到该设备数据

## 技术栈

- **C++11**（全项目，禁止 C++14/17 语法）、CMake 3.16+、Qt **5.15**（Widgets、Network、Charts）
- MySQL 8.x（业务库名建议 `scada_demo`）
- 开发：Windows + Qt Creator（MinGW）；主站部署目标 Linux

## Qt 界面（强制）

- **布局与静态控件**：必须用 **Qt Designer** 编辑 `qt-client/ui/*.ui`，禁止在 `.cpp` 里手写整套 `QVBoxLayout`/`new QPushButton` 搭主界面。
- **混合用法**：`.ui` 负责布局与 `objectName`；`.cpp` 负责业务、网络、告警样式、动态列表项；动态控件挂到 `.ui` 中的容器 `QWidget`。
- **CMake**：每个窗口/页面将 `.ui` 列入 `add_executable`，保持 `AUTOUIC ON`。
- **例外**：`Ui::MainWindow* ui` 可用 `new` + `setupUi`（Qt 惯例）；其余堆对象优先智能指针。

详见 [docs/coding-standards.md](docs/coding-standards.md)。

## 代码放置约定

| 模块 | 路径 | 说明 |
|------|------|------|
| 公共类型/协议 | `common/include/scada/` | 禁止三端复制协议定义 |
| 主站入口 | `master-server/src/app/MasterApplication.*` | `main.cpp` 只做信号注册 |
| 主站网络/104 | `master-server/src/net/` | |
| 主站存库 | `master-server/src/storage/` | |
| 主站推送 | `master-server/src/push/` | |
| 主站业务流水线 | `master-server/src/pipeline/` | 告警、遥控路由 |
| 模拟器 | `device-simulator/src/app/`, `src/sim/` | |
| Qt 界面布局 | `qt-client/ui/*.ui` | Designer 编辑 |
| Qt 页面逻辑 | `qt-client/src/pages/` + 同名 `.ui` | |
| Qt 网络 | `qt-client/src/net/MasterClient.*` | |
| SQL 脚本 | `docs/sql/` | schema.sql、seed.sql |
| 迭代 8 脚本 | `scripts/run_demo.ps1` | |

## 编码规范

- **注释**：新增/修改的每个 `.h/.cpp` 须含 `@file`/`@brief`；公共类与函数写清参数、返回值；关键逻辑写中文块注释（见 coding-standards）。
- **C++11**：不用 `std::optional`、`inline` 变量、嵌套命名空间 `a::b`；可选结果用 `bool` + 出参。
- 主站/模拟器：智能指针 + RAII；Qt 业务对象除 `Ui::*` 外尽量少裸 `new`。
- 104 与 Socket 在主站线程；Qt 控件仅在 UI 线程更新，跨线程用信号槽。
- 领域命名与表字段一致：`device_code`、`RTU001` 等。
- 日志前缀：`[master-server]`、`[device-simulator]`、`[qt-client]`。

## 构建与联调

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="D:/soft/Qt/5.15.2/mingw81_64" -G "MinGW Makefiles" `
  -DCMAKE_MAKE_PROGRAM="D:/soft/Qt/Tools/mingw810_64/bin/mingw32-make.exe"
cmake --build build
```

启动顺序：**device-simulator → master-server → qt-client**。

演示登录（迭代 8）：`admin` / `123456`，主站内存校验，无用户表。

## 实现进度

| 迭代 | 状态 |
|------|------|
| 0 脚手架 | 完成 |
| 1 MySQL 基座 | 完成 |
| 2 IEC104 单设备 | 未开始 |
| 3 Qt 实时监控 | 未开始 |
| 4 三设备在线 | 未开始 |
| 5 设备维护 | 未开始 |
| 6 遥控日志 | 未开始 |
| 7 告警 | 未开始 |
| 8 曲线登录联调 | 未开始 |

## AI 工作流

1. 用户指定迭代 N → 打开 `docs/roadmap.md` 对应章节  
2. 仅实现该迭代「交付物」，不做「不做」章节内容  
3. 需求变更 → 先改 `docs/requirements.md` / `roadmap.md`，再写代码  
4. 完成后更新上表 + roadmap 复选框  

## 边界（禁止擅自）

- 在 Qt `.cpp` 中手写完整主界面布局（须用 `.ui`）  
- 使用 C++14/17 特性或擅自升级标准  
- 提交无文件头/无公共 API 说明的“裸代码”  
- Qt 进程连接 2404 或嵌入 104 协议栈  
- 业务数据写入 `qt-client/data/client_config.db`  
- 用户表、权限、遥调、真实硬件  
- 改为 SQLite 作业务库、或去掉 IEC104/MySQL  
- 修改 `build/`、`.qtcreator/`、`temp/`  
- 未经要求新增大量 markdown（`docs/` 除外）
