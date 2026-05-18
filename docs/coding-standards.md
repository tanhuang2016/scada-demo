# 编码与界面规范

## C++ 标准

- 全项目统一 **ISO C++11**（`CMakeLists.txt` 中 `CMAKE_CXX_STANDARD 11`）。
- 不使用 C++14/17 专属语法（如 `std::optional`、嵌套命名空间 `a::b`、`inline` 变量、`if constexpr` 等）。
- 需要“可选返回值”时用 `bool` + 出参，或 `bool` 表示成败。

## 注释要求

所有 **新增与修改** 的 `.h/.hpp/.cpp` 须带完整注释，便于学习与面试讲解。

### 文件头（每个源文件）

```cpp
/**
 * @file    MasterApplication.cpp
 * @brief   主站应用生命周期：加载配置、启动网络服务、优雅退出
 * @module  master-server
 */
```

### 类

```cpp
/**
 * @brief 主站应用入口，协调配置加载与后台服务
 */
class MasterApplication { ... };
```

### 公共函数

```cpp
/**
 * @brief 解析一行遥测文本协议
 * @param line  原始报文，如 TELEM|RTU001|220|10|1|0
 * @param out   解析成功时写入
 * @return      解析是否成功
 */
bool decodeTelemetry(const std::string& line, Telemetry& out);
```

### 实现要点

- 非平凡逻辑、状态机、协议字段、线程边界处写**行内或块注释**。
- 注释说明 **为什么**（业务/协议原因），不只复述代码。
- 公开 API 用中文或中英均可，保持项目内统一（本项目默认**中文**）。

## Qt 界面规范

### 原则：UI 设计器为主，代码为辅

| 内容 | 做法 |
|------|------|
| 窗口、布局、按钮、标签、表格列 | **Qt Designer** → `qt-client/ui/*.ui` |
| 信号槽连接（简单） | 可在 Designer 中连接，或在 `.cpp` 的 `setupUi` 之后 |
| 业务逻辑、网络、数据库 | `.cpp` / 独立类，**不写**大段 `new QVBoxLayout` |
| 动态控件（运行时数量不定） | 代码创建，父控件在 `.ui` 中预留 `QWidget#container` |
| 样式/告警红闪 | 代码 `setStyleSheet` 或 QSS 文件，不强行塞进 .ui |

### 目录

```text
qt-client/
├── ui/              # .ui 文件（Designer 编辑）
├── src/
│   ├── pages/       # 各页面 Widget 子类 + 对应 ui/*.ui
│   ├── net/
│   └── MainWindow.*
└── resources/       # .qrc、图标（按需）
```

### 类与 .ui 命名

- `ui/MonitorPage.ui` → 类 `MonitorPage`，生成 `ui_MonitorPage.h`
- 头文件中使用前向声明 `namespace Ui { class MonitorPage; }`，成员 `Ui::MonitorPage* ui;`
- 在构造函数中：`ui = new Ui::MonitorPage; ui->setupUi(this);`

### 禁止

- 在 `.cpp` 里从零手写整套主界面布局（迭代 0 临时桩已改为 .ui，后续页面同理）。
- 在 `.ui` 里写业务逻辑（仅布局与 objectName）。

## 主站 / 模拟器

- 无 Qt UI，保持控制台 + 清晰日志。
- 同样遵守 C++11 与注释规范。

## 其他约定

- 命名：类 `PascalCase`，成员 `camelCase_` 或 `m_` 前缀二选一（Qt 侧推荐 `xxx_`）。
- 错误：可恢复错误打日志并返回 `false`；不空 catch。
- MySQL/104 相关魔数放入配置或 `config_defaults.hpp`，避免散落。
