# SCADA Demo

极简电力 SCADA 演示系统（C++ 主站 + Qt 界面 + 设备模拟器），用于学习与面试演示。

## 目录

| 模块 | 说明 |
|------|------|
| `common/` | 公共协议、类型、端口常量 |
| `master-server/` | 主站后台（`MasterApplication`） |
| `device-simulator/` | 现场设备模拟（`SimulatorApplication`） |
| `qt-client/` | Qt 5.15 监控界面（`MainWindow`） |

## 构建（Windows）

1. 安装 Qt 5.15（含 MSVC 或 MinGW 套件）与 CMake 3.16+
2. 用 Qt Creator 打开根目录 `CMakeLists.txt`，选择套件后构建  
   或命令行：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="<你的Qt路径>/5.15.2/msvc2019_64"
cmake --build build
```

## 运行（脚手架）

当前为三端占位程序，无真实网络联调：

```powershell
.\build\bin\master-server.exe
.\build\bin\device-simulator.exe
.\build\bin\qt-client.exe
```

按 Ctrl+C 结束后台进程。

## 术语

- **主站**：`master-server`，汇聚数据、存库、转发遥控
- **子站/设备**：`device-simulator` 模拟
- **四遥**：遥测、遥信、遥控（本演示暂不实现遥调）

详细协议与 AI 协作约定见 [AGENTS.md](AGENTS.md)。
