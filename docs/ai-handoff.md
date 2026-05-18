# 给其他 AI 工具的项目交接说明

将本文件与 [AGENTS.md](../AGENTS.md) 一起提供给 Claude Code、Copilot、Codex 等，即可在少重复解释的情况下继续开发。

## 1. 开场必 @ 的文件（按顺序）

```
AGENTS.md
docs/roadmap.md          ← 指定要做「迭代 N」时，只读该迭代章节
docs/coding-standards.md
docs/requirements.md     ← 需要查范围/验收时
docs/data-model.md       ← 迭代 1+ 涉及表结构时
docs/architecture.md
```

## 2. 可直接复制的任务提示词（模板）

将下面 `{N}` 换成迭代号（当前下一步为 **1**）：

```text
你在仓库 E:/code/cpp/scada-demo 中开发电力 SCADA 演示项目。

【必读】
1. 先完整阅读 AGENTS.md 和 docs/coding-standards.md，严格遵守。
2. 只实现 docs/roadmap.md 中「迭代 {N}」的「交付物」和「完成标准」。
3. 不要做该迭代「不做」章节中的内容，不要擅自改架构（104+MySQL+Qt.ui 已定型）。

【技术约束摘要】
- C++11 only；注释完整（@file/@brief/关键逻辑中文注释）
- Qt：布局用 qt-client/ui/*.ui（Designer），禁止手写整套主界面布局
- 设备通信：IEC104:2404（仅主站与模拟器）；主站↔Qt：TCP 5002/5003 文本帧
- 业务库 MySQL；Qt 本地 SQLite 仅界面偏好

【本次任务】
实现迭代 {N}：……（可粘贴 roadmap 里该迭代的「目标」一行）

【完成后必须】
1. 在 docs/roadmap.md 勾选本迭代完成标准
2. 更新 AGENTS.md 底部「实现进度」表
3. 说明如何构建、如何按 roadmap 演示步骤验收
```

## 3. 各工具用法

| 工具 | 建议 |
|------|------|
| **Claude Code** | 在仓库根建 `CLAUDE.md`，写一句：`请先阅读 AGENTS.md 与 docs/roadmap.md，所有开发遵守其中约定。` |
| **Cursor** | 自动读 `AGENTS.md`；对话里 @ `docs/roadmap.md` 并说明迭代号 |
| **GitHub Copilot** | 同读根目录 `AGENTS.md`（Agentic AI 标准） |
| **Codex CLI** | 根目录 `AGENTS.md` 即项目说明 |

## 4. 当前项目状态（交接时请自行核对并改掉过期描述）

| 项 | 状态 |
|----|------|
| 迭代 0 脚手架 | 已完成 |
| 下一迭代 | **迭代 1 — MySQL 基座** |
| 构建 | Qt 5.15.2 MinGW，CMake 打开根 `CMakeLists.txt` |

## 5. 好的任务拆分示例

- ✅「实现迭代 1：建 docs/sql/schema.sql、seed.sql，主站启动打印 3 台设备」
- ✅「实现迭代 3：主站 push 5002 + Qt MonitorPage.ui 实时刷新」
- ❌「把整个 SCADA 做完」（范围太大，难以验收）
- ❌「改用 Web 前端 / SQLite 存业务 / Qt 直连 104」（违反 AGENTS 边界）

## 6. 环境信息（按需补充给 AI）

```text
OS: Windows
Qt: 5.15.2，路径示例 D:/soft/Qt/5.15.2/mingw81_64
MySQL: （填写你的 host/port/用户，如 127.0.0.1:3306 root/xxx，库名 scada_demo）
```
