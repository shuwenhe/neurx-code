# Agent 文件写入功能 - 完整实现指南

## 概述

neurx-code agent 现已支持完整的文件写入功能，包括：
- ✅ 单个文件写入
- ✅ 批量文件写入（原子操作）
- ✅ 文件内容更新（追加、前置、插入）
- ✅ 模板文件生成
- ✅ 目录结构创建
- ✅ 自动备份和检查点

---

## 架构概览

### 文件写入组件

```
Agent System
    ↓
Agent Planner (规划)
    ↓
Tool Registry (工具注册)
    ├─ FileSystemTool (基础文件操作)
    └─ AgentFileWriterTool (Agent 专用文件写入) ← 新增
    ↓
Executor (执行)
    ↓
File System (文件系统)
```

### 核心工具

| 工具 | 功能 | 操作 |
|------|------|------|
| `FileSystemTool` | 基础文件操作 | read_file, write_file, create_file, delete_file |
| `AgentFileWriterTool` | Agent 文件写入 | write_single, write_batch, update_file, write_template, create_structure |

---

## 快速开始

### 1. 单个文件写入

**Agent Plan:**
```
任务：创建项目文件
步骤1：写入 main.py
步骤2：写入 requirements.txt
步骤3：写入 README.md
```

**Tool Call (write_single):**
```json
{
  "tool": "agent_file_writer",
  "callId": "write_1",
  "args": {
    "operation": "write_single",
    "path": "src/main.py",
    "content": "#!/usr/bin/env python3\nprint('Hello, World!')",
    "create_dirs": true,
    "backup": true,
    "checkpoint": true
  }
}
```

**Response:**
```
Successfully wrote: src/main.py
Size: 42 bytes
Backup created: src/main.py.backup.20260611_120000
Checkpoint ID: cp_abc123def456
```

---

### 2. 批量文件写入（原子操作）

**Agent Plan:**
```
任务：创建完整项目结构
操作：原子性批量写入 5 个文件
```

**Tool Call (write_batch):**
```json
{
  "tool": "agent_file_writer",
  "callId": "write_batch_1",
  "args": {
    "operation": "write_batch",
    "atomic": true,
    "checkpoint": true,
    "files": [
      {
        "path": "src/main.py",
        "content": "#!/usr/bin/env python3\nimport click\n\n@click.command()\ndef main():\n    click.echo('Hello')\n\nif __name__ == '__main__':\n    main()"
      },
      {
        "path": "src/utils.py",
        "content": "def helper_function():\n    return 'helper result'"
      },
      {
        "path": "requirements.txt",
        "content": "click==8.1.0\nrequests==2.28.0"
      },
      {
        "path": "README.md",
        "content": "# My Project\n\nDescription here"
      },
      {
        "path": ".gitignore",
        "content": "__pycache__/\n*.pyc\n.env\ndist/"
      }
    ]
  }
}
```

**Response:**
```
Batch write completed: 5/5 files written
Checkpoint ID: cp_batch_001
```

---

### 3. 文件内容更新

**场景：向现有文件添加内容**

**Tool Call (update_file - append):**
```json
{
  "tool": "agent_file_writer",
  "callId": "update_1",
  "args": {
    "operation": "update_file",
    "path": "README.md",
    "content": "\n## Installation\n\nRun: pip install -r requirements.txt",
    "mode": "append"
  }
}
```

**支持的 mode 参数：**
- `append` - 添加到文件末尾
- `prepend` - 添加到文件开头
- `insert` - 在指定行插入（需要 line 参数）
- `overwrite` - 完全替换文件内容

---

### 4. 模板文件生成

**预置模板：**
- `python-cli` - Python CLI 应用模板
- `requirements` - Python 依赖文件模板
- `readme` - README 模板

**Tool Call (write_template):**
```json
{
  "tool": "agent_file_writer",
  "callId": "template_1",
  "args": {
    "operation": "write_template",
    "path": "src/cli.py",
    "template_name": "python-cli",
    "variables": {
      "PROJECT_NAME": "MyApp",
      "PROJECT_DESC": "My awesome application"
    }
  }
}
```

**Template Output:**
```python
#!/usr/bin/env python3
"""MyApp - My awesome application"""

import argparse
import sys

def main():
    parser = argparse.ArgumentParser(description="My awesome application")
    parser.add_argument("--version", action="version", version="%(prog)s 1.0")
    args = parser.parse_args()
    
    print("MyApp initialized")

if __name__ == "__main__":
    main()
```

---

### 5. 创建目录结构

**场景：一次性创建整个项目结构**

**Tool Call (create_structure):**
```json
{
  "tool": "agent_file_writer",
  "callId": "struct_1",
  "args": {
    "operation": "create_structure",
    "structure": {
      "src": {
        "main.py": "# Main application",
        "utils.py": "# Utility functions",
        "config.py": "# Configuration"
      },
      "tests": {
        "test_main.py": "import unittest\n\nclass TestMain(unittest.TestCase):\n    pass"
      },
      "docs": {
        "README.md": "# Documentation"
      },
      "requirements.txt": "click==8.1.0"
    }
  }
}
```

**Response:**
```
Created structure: 3 directories, 6 files
```

---

## 深入理解：Agent 执行流程

### 步骤 1: 规划（Planning）

Agent LLM 根据用户请求生成计划：

```plaintext
User Request:
"创建一个 Python 项目，包含 CLI 工具、测试和文档"

Agent Plan:
1. 分析项目需求
   - CLI 应用
   - 单元测试
   - README 文档

2. 设计文件结构
   - src/main.py (CLI 入口)
   - src/utils.py (工具函数)
   - tests/test_main.py (测试)
   - README.md (文档)
   - requirements.txt (依赖)
   - .gitignore (Git 配置)

3. 执行文件创建任务
   - 使用 write_batch 原子写入所有文件
   - 创建检查点以支持回滚
```

### 步骤 2: 工具注册和发现

Agent 在 Tool Registry 中发现可用工具：

```cpp
// 在 AgentToolRegistry 中注册
auto fileWriter = std::make_shared<AgentFileWriterTool>(workspaceRoot);
registry->registerTool(fileWriter);

// Agent 发现工具
auto tools = registry->getAvailableTools();
// 返回: ["agent_file_writer", "file_system", ...]
```

### 步骤 3: 生成 Tool Call

Agent 根据计划生成 Tool Call JSON：

```json
{
  "type": "tool_use",
  "id": "toolu_abc123",
  "name": "agent_file_writer",
  "input": {
    "operation": "write_batch",
    "files": [
      { "path": "src/main.py", "content": "..." },
      { "path": "README.md", "content": "..." }
    ],
    "atomic": true,
    "checkpoint": true
  }
}
```

### 步骤 4: 执行和反馈

Executor 执行 Tool Call：

```cpp
// 执行工具
ToolResult result = fileWriter->execute(callId, toolArgs);

if (result.isError) {
    // 错误处理
    agent->handleError(result.message);
} else {
    // 成功反馈给 Agent
    agent->processResult(result);
    
    // Agent 可能继续规划下一步
}
```

---

## 完整工作流示例

### 场景：创建 MCP 服务器项目

**User Request:**
```
创建一个 GitHub MCP 服务器项目，包含：
- 基础项目文件（main.py、requirements.txt）
- 测试框架
- 完整文档
- GitHub Actions 工作流
```

**Agent Execution Flow:**

```
1️⃣ ANALYZE REQUIREMENTS
   → 分析用户需求
   → 确定所需文件：5 个 Python 文件，2 个配置文件，3 个文档

2️⃣ PLAN STRUCTURE
   → 设计项目结构
   → 规划文件创建顺序

3️⃣ BATCH WRITE (原子操作)
   ├─ src/main.py (MCP 服务器主文件)
   ├─ src/handlers.py (GitHub 事件处理器)
   ├─ src/config.py (配置)
   ├─ tests/test_main.py (单元测试)
   ├─ requirements.txt (依赖)
   ├─ README.md (文档)
   ├─ .github/workflows/test.yml (CI/CD)
   └─ .gitignore (Git 配置)
   
   Response: ✅ 8/8 files written
            Checkpoint ID: cp_mcp_project_001

4️⃣ VERIFY & SUMMARIZE
   → 验证所有文件创建成功
   → 向用户报告结果
   → 提供后续建议
```

---

## 安全特性

### 1. 路径遍历保护

```cpp
// ❌ 被阻止
"path": "../../sensitive/file.txt"  // 遍历攻击
"path": "/etc/passwd"               // 绝对路径

// ✅ 允许
"path": "src/main.py"               // 相对路径
"path": "tests/test_utils.py"       // 嵌套相对路径
```

### 2. 原子性保证

**批量写入时：**
- 所有文件写入成功，或全部失败
- 不会产生部分写入状态
- 支持回滚到检查点

### 3. 备份机制

```json
{
  "operation": "write_single",
  "path": "important.txt",
  "content": "new content",
  "backup": true  // 自动创建 important.txt.backup.20260611_120000
}
```

### 4. 内容验证

- 最大文件大小：10 MB
- 检测空字节
- 编码验证

---

## 错误处理

### 常见错误和解决方案

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| "Path traversal attack detected" | 路径包含 `../` | 使用相对路径 |
| "Cannot create directories" | 权限不足 | 检查工作目录权限 |
| "Content too large" | 超过 10 MB | 分割文件或调整限制 |
| "Batch write aborted" | 原子模式下写入失败 | 检查单个文件错误 |

### 错误恢复

```json
{
  "operation": "write_batch",
  "atomic": false,  // 部分失败时继续
  "files": [...]
}

// Response 包含部分成功信息和失败详情
```

---

## 集成到 Agent 系统

### 1. 在 Tool Registry 中注册

```cpp
// agent/AgentToolRegistry.cpp
#include "tools/AgentFileWriterTool.h"

void AgentToolRegistry::initializeDefaultTools()
{
    // ... 其他工具 ...
    
    // 注册 Agent 文件写入工具
    auto fileWriter = std::make_shared<AgentFileWriterTool>(m_workspaceRoot);
    registerTool(fileWriter);
}
```

### 2. 在 CMakeLists.txt 中添加源文件

```cmake
target_sources(neurx_core PRIVATE
    # ... 其他源文件 ...
    src/tools/AgentFileWriterTool.cpp
)
```

### 3. 在 Executor 中处理 Tool Results

```cpp
// execution/DefaultExecutionEngine.cpp
void DefaultExecutionEngine::onToolResultReady(const ToolResult &result)
{
    if (result.toolName == "agent_file_writer") {
        // 文件写入完成
        if (!result.isError) {
            // 提取检查点 ID
            // 更新 UI
            // 继续执行计划
        } else {
            // 处理错误
            // 可能需要回滚
        }
    }
}
```

---

## 性能考虑

### 批量写入优化

```json
{
  "operation": "write_batch",
  "atomic": true,        // 启用原子操作
  "checkpoint": true,    // 创建检查点
  "files": []            // 最多 100 个文件
}
```

### 性能指标

- 单文件写入：~5-10ms
- 批量 50 个文件：~100-150ms
- 目录结构创建：~50-100ms

---

## 最佳实践

### 1. 使用原子操作进行相关文件写入

❌ 不好：多次调用 write_single
```json
{ "operation": "write_single", "path": "file1.py", ... }
{ "operation": "write_single", "path": "file2.py", ... }
{ "operation": "write_single", "path": "file3.py", ... }
```

✅ 好：使用 write_batch
```json
{ "operation": "write_batch", "atomic": true, "files": [...] }
```

### 2. 为重要文件启用备份

```json
{
  "operation": "write_single",
  "path": "config.json",
  "backup": true  // 修改前创建备份
}
```

### 3. 始终使用检查点

```json
{
  "operation": "write_batch",
  "checkpoint": true  // 支持恢复到之前的状态
}
```

### 4. 验证目录结构创建

```json
{
  "operation": "create_structure",
  "structure": {
    "src": { "main.py": "..." },
    "tests": { "test.py": "..." }
  }
  // 返回 "Created structure: 2 directories, 2 files"
}
```

---

## 调试和监控

### 查看文件写入日志

```cpp
// 在 Executor 中启用日志
logger->info("Tool Call: {}", toolCall.name);
logger->info("Operation: {}", args["operation"]);
logger->info("Path: {}", args["path"]);

// 执行后
logger->info("Result: {}", result.message);
```

### 查看检查点

```cpp
// 列出所有检查点
auto checkpoints = checkpointManager->listCheckpoints();
for (const auto &cp : checkpoints) {
    logger->info("Checkpoint {} created at {}", cp.id, cp.timestamp);
}
```

---

## 未来增强

计划增加的功能：
- [ ] 支持二进制文件写入（图像、PDF 等）
- [ ] 文件加密和权限管理
- [ ] 版本控制集成（Git 自动提交）
- [ ] 文件差异显示和三向合并
- [ ] 分布式文件系统支持
- [ ] 事务日志和完全回滚

---

## 总结

neurx-code 现已具备完整的 Agent 文件写入功能，支持：

✅ 单个、批量、模板、结构化文件写入  
✅ 自动备份和检查点机制  
✅ 原子性操作保证  
✅ 安全的路径验证  
✅ 内容验证和错误处理  

使用这些功能，Agent 可以实现复杂的文件创建和修改任务，从项目脚手架到代码生成。
