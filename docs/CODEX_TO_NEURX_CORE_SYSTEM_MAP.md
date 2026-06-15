# Codex Core Systems -> NeurX-Code 对照表

这份表用于回答一个更具体的问题：Codex agent 的核心系统，NeurX-Code 现在对应到哪一步了。

状态说明：

- `已实现`：仓库里已经有明确代码实现和稳定入口
- `部分实现`：有基础设施或局部能力，但还没有形成完整的 Codex 体验
- `待补`：当前仓库里看不到对应的完整实现

## 1. 核心系统总览

| Codex 核心系统 | NeurX-Code 对应实现 | 状态 | 备注 |
|---|---|---|---|
| 任务/线程编排 | `AgentEngine`、`AgentController`、`TaskSession` 相关文档/状态管理 | 部分实现 | 有对话历史、执行循环、状态机，但“稳定 threadId + 分支恢复”还不够完整 |
| 代码变更系统 | `FileSystemTool`、`PatchTool`、`FileService` | 已实现 | 已有 `read_many`、`replace_batch`、`preview_patch`、`apply_patch`、`diff`、`tree`、`watch/unwatch` 等能力 |
| 工具调用系统 | `ToolRegistry`、`AgentToolRegistry`、`ToolTypes` | 已实现 | 覆盖注册、发现、执行、链式调用、校验、权限、统计、配置、市场集成 |
| 安全与审批系统 | `SandboxManager`、`ApprovalManager`、`FolderTrustManager` | 已实现 | 具备沙箱策略、路径控制、只读模式、审批流、受信任文件夹管理 |
| 上下文系统 | `ContextManager`、`WorkspaceContext`、`WorkspaceIndex` | 已实现 | 已经能把文件、工作区、编辑器上下文串起来 |
| 扩展与集成系统 | `MCPManager`、`SlashCommandManager`、`RuleEngine`、`EventBus`、`SkillManager` | 已实现 | 具备命令、规则、事件、技能、外部工具接入的基础架构 |
| 文件系统抽象 | `ExecutorFileSystem`、`DirectFileSystem`、`SandboxedFileSystem`、`LocalFileSystem` | 已实现 | 这是最接近 Codex 文件层架构的部分 |
| Headless / SDK 入口 | 现有文档和桌面入口 | 待补 | 目前主线仍偏桌面应用，批处理/CLI/SDK 入口还不完整 |
| 结构化输出 | 当前 agent/工具返回值 | 部分实现 | 有 JSON 和工具结果结构，但还没看到完整的 schema 驱动任务输出层 |
| 图片/截图输入 | `ScreenshotTool`、附件/上下文注入 | 部分实现 | 有图像与附件入口，但还没有完整的“多模态任务层”体验 |
| 任务级网络控制 | `SandboxManager` 网络策略 | 部分实现 | 有策略骨架，但是否达到 Codex 那种任务级控制还要继续补齐 |

## 2. 对照结论

NeurX-Code 现在不是“只有一个 agent 壳子”，而是已经有了 Codex 风格的几层核心能力：

- 文件系统层已经比较完整
- 工具层、权限层、沙箱层已经成体系
- 命令、规则、技能、MCP 这些扩展层也已经搭起来了

当前最像 Codex 但还没完全补齐的，主要是：

- 稳定的线程/分支恢复
- 更强的结构化任务输出
- 更完整的 headless / SDK 入口
- 更细粒度的任务级网络与执行策略

## 3. 建议继续补的顺序

1. 线程恢复和分支元数据
2. 任务级 sandbox / approval policy
3. 结构化 execution timeline
4. 文件变更 review / rollback 体验
5. Headless / CLI / SDK 入口

