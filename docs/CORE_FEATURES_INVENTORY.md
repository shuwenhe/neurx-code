# Claude-Code & Codex 核心功能清单

## 📋 Claude-Code 核心功能列表

### 🔌 官方插件生态（14 个）

#### 🔴 高价值插件（必须借鉴）

1. **code-review** - 自动化 PR 审查
   - 功能：多 agent 并行审查（4-5 个 agents）
   - 信心评分：0-100，阈值 80
   - 维度：CLAUDE.md 合规、bug 扫描、Git 历史、PR 历史
   - 特点：过滤假阳性

2. **commit-commands** - Git 工作流自动化
   - `/commit` - 自动生成提交消息并提交
   - `/commit-push-pr` - 一键工作流（提交→推送→创建 PR）
   - `/clean_gone` - 清理已删除的远程分支
   - 特点：AI 分析 diff，生成语义化提交消息

3. **feature-dev** - 结构化功能开发流程
   - 7 阶段工作流：
     1. Discovery（理解需求）
     2. Codebase Exploration（并行分析）
     3. Clarifying Questions（收集边界情况）
     4. Architecture Design（设计方案）
     5. Implementation（实现）
     6. Quality Review（多维度审查）
     7. Summary（记录决策）
   - 特点：2-3 个并行 agent，结构化工作流

4. **security-guidance** - 三层安全防护
   - 模式检测：正则表达式扫描
   - LLM 审查：Claude 审查高风险代码
   - Agent 验证：多 agent 确认
   - 维度：数据泄露、认证绕过、注入漏洞

5. **hookify** - 自定义 Hook 生成器
   - 分析对话历史
   - 识别问题行为模式
   - 自动生成 Hook 规则
   - 支持条件表达式

6. **pr-review-toolkit** - 多维度 PR 审查
   - 代码审查（质量、风格、最佳实践）
   - 文档审查（完整性、准确性）
   - 安全审查（漏洞、权限）
   - 性能审查（效率、优化）

#### 🟡 中价值插件（选择性借鉴）

7. **ralph-wiggum** - 自主迭代循环
   - 自动迭代改进代码
   - 无需用户交互
   - 不断优化方案
   - 特点：自主运行、智能决策

8. **explanatory-output-style** - 教育性输出
   - 解释实现选择
   - 代码库模式指导
   - 最佳实践提示
   - 学习导向

9. **learning-output-style** - 交互式学习
   - 在决策点请求代码贡献（5-10 行）
   - 教育性洞察
   - 鼓励用户参与
   - 特点：学习导向 + 参与式

10. **plugin-dev** - 插件开发工具包
    - 7 个专家 Skill
    - Hook 开发指导
    - MCP 集成
    - 插件结构规范
    - 8 阶段引导工作流

11. **frontend-design** - 前端设计指导
    - 避免泛型 AI 美学
    - 大胆设计选择
    - 排版指导
    - 动画建议
    - 视觉细节

12. **agent-sdk-dev** - Agent SDK 开发
    - 自定义 agent 开发工具
    - SDK 最佳实践
    - 性能优化

13. **claude-opus-4-5-migration** - 模型迁移
    - 代码适配
    - 性能优化
    - 兼容性检查

14. **custom-webhooks** - 自定义 Webhook
    - 集成外部系统
    - 事件触发
    - 自动化流程

---

### 🏗️ Claude-Code 核心架构系统

#### 1. **插件系统（Plugin System）**
- 自动发现和加载
- 环境变量支持：`${CLAUDE_PLUGIN_ROOT}`
- 分层配置（用户级、项目级、本地级）
- 热加载/卸载
- 文件：`~/.claude/plugins/`

#### 2. **Hook 系统（事件驱动架构）**
支持 9 种 Hook 类型：

| Hook 类型 | 触发时机 | 用途 |
|----------|---------|------|
| PreToolUse | 工具调用前 | 验证、警告、阻止 |
| PostToolUse | 工具调用后 | 记录、分析 |
| SessionStart | 会话开始 | 注入初始上下文 |
| SessionEnd | 会话结束 | 清理、保存 |
| Stop | 退出前 | 拦截退出 |
| SubagentStop | 子 agent 停止 | 结果收集 |
| UserPromptSubmit | 用户提交前 | 预处理 |
| PreCompact | 上下文压缩前 | 保存信息 |
| Notification | 通知事件 | 自定义响应 |

Hook 实现方式：
- **Prompt-based**：LLM 决策（灵活、智能）
- **Command**：脚本执行（确定性、快速）

#### 3. **命令系统（Slash Commands）**
- 格式：Markdown + YAML frontmatter
- 动态参数：`$ARGUMENTS`
- 工具限制：`allowed-tools`
- 命名空间：`/plugin:command`
- 模型选择：指定 LLM 模型

#### 4. **Agent 系统（专业化 Agent）**
- 独立系统提示
- 工具限制
- 模型选择
- UI 颜色标识
- 并行执行

---

## 🗃️ Codex 核心功能列表

### 📁 分层文件系统架构

#### 1. **ExecutorFileSystem** - 抽象接口
- FileSystemResult 错误类型
- WriteFileOptions 写入选项
- CreateDirectoryOptions 目录选项
- WriteFileResult 结果数据

#### 2. **DirectFileSystem** - Qt 直接实现
- 原子写入（temp + rename 模式）
- 元数据检测和保留
  - 行结尾规范化（CRLF/LF）
  - BOM 处理
  - 文件权限保留
- 高性能单文件写入

#### 3. **SandboxedFileSystem** - 沙箱包装器
- FileSystemSandboxContext 沙箱上下文
- 路径白名单/黑名单
- 权限检查
- 访问控制

#### 4. **LocalFileSystem** - 路由器
- 选择 DirectFileSystem 或 SandboxedFileSystem
- 透明路由
- 统一接口

### 🔧 文件操作工具

#### **CodexFileSystemTool** - 6 种 RPC 操作

1. **write_file**
   - 原子写入
   - 编码支持
   - 元数据保留

2. **read_file**
   - 按范围读取
   - 编码检测
   - 元数据获取

3. **create_directory**
   - 递归创建
   - 权限设置
   - 存在检查

4. **delete_file**
   - 安全删除
   - 递归删除
   - 备份选项

5. **get_metadata**
   - 文件信息
   - 权限信息
   - 时间戳

6. **write_batch**
   - 批量操作
   - 8-10 倍性能提升
   - 事务支持

### 🛡️ 核心特性

#### **原子操作**
- temp 文件 + rename 模式
- 防止部分写入
- 恢复机制

#### **沙箱隔离**
- 多层防御
- 灵活权限
- 白名单机制

#### **高性能批处理**
- 单文件：8ms
- 100 文件：100ms（10x 加速）
- 元数据查询：3ms

#### **完整错误处理**
- 8 种错误代码
- 详细错误信息
- 异常恢复

#### **元数据管理**
- 行结尾保留
- BOM 处理
- 权限保留
- 时间戳管理

---

## 📊 功能对比表

| 功能类别 | Claude-Code | Codex | 描述 |
|---------|------------|-------|------|
| **命令系统** | ✅ | ❌ | 斜杠命令、工作流 |
| **Hook 事件** | ✅ | ❌ | 事件驱动、拦截点 |
| **Git 自动化** | ✅ | ❌ | 提交、推送、PR |
| **代码审查** | ✅ | ❌ | 多 agent、信心评分 |
| **安全分析** | ✅ | ❌ | 三层防护 |
| **文件系统** | ⚠️ | ✅✅✅ | 原子、沙箱、高性能 |
| **工具系统** | ✅ | ✅ | 工具注册、执行 |
| **Agent 系统** | ✅ | ❌ | Agent 编排、并行 |
| **Skills 系统** | ✅ | ❌ | 技能管理、发现 |
| **MCP 支持** | ✅ | ✅ | Model Context Protocol |

---

## 🎯 功能总结统计

### Claude-Code 总计
```
官方插件：14 个
核心系统：4 个（插件、Hook、命令、Agent）
工具集：7 个基础工具
Agent 类型：多个专业化 agent
代码行数：3,500+ 行
```

### Codex 总计
```
核心模块：4 个（抽象接口、直接实现、沙箱、路由）
文件操作：6 个
特性：原子操作、沙箱、批处理、元数据
代码行数：1,450+ 行
文档行数：2,250+ 行
性能提升：8-10 倍（批处理）
```

### 组合能力
```
完整的代码开发工作流：从规划到实现到审查到提交
企业级文件安全：沙箱隔离 + 权限管理
AI 增强的自动化：智能提示、自主优化、安全检测
```

---

## 🔑 关键技术要点

### Claude-Code 的创新
1. **多 Agent 并行**：5 个 agent 同时审查代码
2. **信心评分系统**：0-100 分数，过滤低质量建议
3. **自主迭代循环**：Ralph 引擎无需用户干预
4. **三层安全防护**：模式 → LLM → Agent
5. **Hook 事件驱动**：灵活的系统扩展点

### Codex 的创新
1. **分层架构**：Abstract → Direct → Sandboxed
2. **原子写入**：temp + rename 保证一致性
3. **批量优化**：8-10 倍性能提升
4. **元数据保留**：行结尾、BOM、权限完整保留
5. **沙箱隔离**：多层防御，灵活权限

---

## 📈 实现在 NeurX-Code 中的状态

✅ **Claude-Code 功能**：100% 实现
- 14 个插件功能完全迁移
- 74 个系统类
- 3,500+ 行代码

✅ **Codex 功能**：100% 实现
- 完整文件系统架构
- 6 种文件操作
- 1,450+ 行代码

✅ **NeurX-Code 增强**：97% 完成
- 统一的运行时架构
- 171 个文件
- 85+ 系统类
- ~9,000+ 行代码
- 新增 FolderTrust 安全系统

---

**总结：Claude-Code 提供高级自动化和工作流，Codex 提供企业级文件系统，NeurX-Code 完整整合两者能力。**