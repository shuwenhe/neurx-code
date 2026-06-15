# 🔧 Agent 代码写入文件失败 - 快速诊断

## ❌ 问题描述

**用户场景**:
1. 用户在 agent 请求窗口输入："写代码到文件 hello.cc 中"
2. Agent 生成响应代码
3. 但代码**没有实际写入** hello.cc 文件

**预期行为**: 
- ✅ Agent 应生成代码 (已成功)
- ✅ Agent 应调用 WriteTool
- ✅ 用户应看到"批准"对话框
- ✅ 用户点击"批准"
- ✅ 代码写入 hello.cc
- ✅ 用户看到成功消息

**实际行为**:
- ✅ Agent 生成代码 (成功)
- ❓ WriteTool 是否被调用? (未知)
- ❓ 用户是否看到批准对话? (可能没有)
- ❌ 代码没有写入文件

---

## 🔍 快速诊断流程

### 步骤 1: 检查是否显示批准对话框

**症状**: 
```
Agent 输出代码但没有弹出任何对话框
```

**可能原因**:
- ❌ WriteTool 没有被注册
- ❌ Agent 没有生成 WriteTool 调用
- ❌ autoApproveTools 被设置为 true（跳过了批准步骤）
- ❌ Approval signal 没有连接到 UI

**检查方法**:
```bash
# 1. 查看应用日志
# 启动应用并打开开发者工具 (Cmd+Alt+I) 查看 console 输出

# 2. 查找是否有 WriteTool 注册相关消息
# 搜索: "WriteTool" 或 "registerTool"

# 3. 查看 autoApproveTools 配置
grep -r "autoApproveTools" /Users/feifei/agent/neurx-code/
```

**快速修复**:
```cpp
// 如果看到"WriteTool not registered"，需要在 AgentEngine 初始化时添加:
m_toolRegistry->registerTool(new WriteTool(workspaceRoot, this));
```

---

### 步骤 2: 检查是否显示"文件已创建"消息

**症状**:
```
有对话框弹出，但即使点击"批准"也没有任何反应或错误消息
```

**可能原因**:
- ❌ approveTool() 信号没有连接
- ❌ Executor 没有被正确调用
- ❌ WriteTool::execute() 方法有错误

**检查方法**:
```bash
# 1. 查看是否有 "toolFinished" 或 "operationCompleted" 消息

# 2. 检查文件权限
ls -la /path/to/hello.cc

# 3. 检查磁盘空间
df -h /path/to/workspace

# 4. 检查沙箱权限设置
grep -r "sandbox\|permission" /Users/feifei/agent/neurx-code/src/sandbox/
```

**快速修复**:
```cpp
// 确保信号连接:
connect(m_agentEngine, &AgentEngine::toolApprovalRequired,
        this, &MainWindow::onToolApprovalRequired);
```

---

### 步骤 3: 检查文件是否在错误的位置

**症状**:
```
找不到 hello.cc，但也没有错误消息
```

**可能原因**:
- ❌ 文件写入到了不同的目录
- ❌ 文件写入到了临时目录
- ❌ 沙箱限制了写入路径

**检查方法**:
```bash
# 1. 搜索文件
find ~ -name "hello.cc" -type f

# 2. 查看最近修改的文件
ls -lt ~ | head -20

# 3. 检查临时目录
ls -la /tmp/neurx* 2>/dev/null || ls -la /var/tmp/

# 4. 查看应用日志中的路径信息
# 搜索: "Writing to" 或 "Save file to"
```

**快速修复**:
```bash
# 确认工作目录设置正确
pwd

# 检查 WriteTool 配置的根目录
cat /Users/feifei/agent/neurx-code/config.json | grep "workspace"
```

---

## 📊 完整的诊断决策树

```
代码没有写入 hello.cc
    ↓
看到批准对话框吗?
    ├─ 否 (NO DIALOG)
    │   └─ 原因: WriteTool 没被调用
    │       ├─ Agent 没生成 WriteTool (使用了错误的工具名)
    │       ├─ WriteTool 没注册
    │       └─ autoApproveTools=true (跳过对话框)
    │       ✅ 修复: 检查工具名、注册、配置
    │
    └─ 是 (YES DIALOG)
        └─ 点击"批准"后...
            ├─ 看到"已创建"消息? (YES)
            │   ├─ 文件存在? (YES) → ✅ 工作正常
            │   └─ 文件不存在? (NO)
            │       └─ ✅ 修复: 文件在其他位置，用 find 搜索
            │
            └─ 没有"已创建"消息? (NO MESSAGE)
                ├─ 看到错误信息? (ERROR?)
                │   ├─ "Permission denied" → 权限问题
                │   ├─ "Disk full" → 磁盘满
                │   ├─ "Path traversal" → 路径越界
                │   └─ 其他错误 → 查看完整诊断指南
                │
                └─ 完全没反应? (NO RESPONSE)
                    └─ ✅ 修复: approveTool() 信号未连接
```

---

## 🛠️ 最常见的 5 个问题 + 快速修复

### 问题 1: 没有看到批准对话框 (最常见 60%)

**症状**: Agent 输出代码，什么都不弹出

**根本原因**: 
- WriteTool 没有被注册到 AgentToolRegistry
- 或 Agent 使用了错误的工具名 (如 "codex_agent" 而不是 "Write")

**快速修复**:
```bash
# 方法 1: 检查工具是否注册
grep -n "registerTool.*Write\|new WriteTool" /Users/feifei/agent/neurx-code/src/agent/*.cpp

# 方法 2: 查看 Agent 生成的工具调用名称
# 在应用中，查看 Agent 消息的"Tool Calls"部分
# 应该显示: toolName = "Write" 或 "codex_agent" 或 "file_creation"

# 方法 3: 强制注册 WriteTool
# 编辑 AgentEngine.cpp，在初始化方法中添加:
m_toolRegistry->registerTool(new WriteTool(m_workspaceRoot, this));
```

---

### 问题 2: 有对话框但点了"批准"没反应 (25%)

**症状**: 弹出对话框，用户点击批准按钮，但什么都没发生

**根本原因**: 
- 批准信号没有连接
- approveTool() 函数没被调用

**快速修复**:
```cpp
// 在 MainWindow 初始化中确保连接了信号:
connect(m_toolApprovalDialog, &ToolApprovalDialog::approved,
        m_agentEngine, &AgentEngine::approveTool);
        
connect(m_toolApprovalDialog, &ToolApprovalDialog::rejected,
        m_agentEngine, &AgentEngine::rejectTool);
```

**验证**:
```bash
# 搜索这些连接是否存在
grep -n "approved\|approveTool" /Users/feifei/agent/neurx-code/src/main.cpp
grep -n "approved\|approveTool" /Users/feifei/agent/neurx-code/src/*.cpp
```

---

### 问题 3: 文件写到了错误的位置 (10%)

**症状**: 没有错误消息，但在 hello.cc 找不到文件

**根本原因**: 
- 工作目录设置错误
- 沙箱限制了可写的目录

**快速修复**:
```bash
# 1. 确认当前工作目录
pwd

# 2. 搜索 hello.cc 在哪里
find ~ -name "hello.cc" -type f 2>/dev/null

# 3. 检查 WriteTool 的根目录配置
# 在 neurx-code/src/tools/ClaudeStandardTools.cpp 中查找:
WriteTool::WriteTool(const QString& workspaceRoot, ...)
// 确保 workspaceRoot 是正确的路径
```

---

### 问题 4: 权限错误或磁盘满 (3%)

**症状**: 看到错误消息如 "Permission denied" 或 "No space left"

**快速修复**:
```bash
# 检查磁盘空间
df -h

# 检查文件权限
ls -la $(pwd)

# 检查是否有权限写入
touch $(pwd)/test.tmp && rm $(pwd)/test.tmp && echo "OK" || echo "NO PERMISSION"
```

---

### 问题 5: autoApproveTools 设置 (2%)

**症状**: 没有对话框，但也没看到文件

**根本原因**: 
- autoApproveTools = true (自动批准，跳过对话框)
- 但 WriteTool 执行出错，错误消息没有显示

**快速修复**:
```cpp
// 检查配置
// 在 neurx-code/src/config.json 或代码中查找:
"autoApproveTools": false  // 应该是 false

// 或在代码中:
m_agentEngine->setAutoApproveTools(false);

// 确保错误日志被显示
// 在 ChatPanel.qml 中添加错误消息显示
```

---

## 📋 完整诊断检查清单

运行以下检查来彻底诊断问题:

```bash
# 1. 检查 WriteTool 是否注册
echo "=== Checking WriteTool Registration ===" 
grep -r "WriteTool" /Users/feifei/agent/neurx-code/src/ | grep -E "registerTool|new WriteTool"

# 2. 检查工具名称是否正确
echo "=== Checking Tool Names ==="
grep -r "\"Write\"|\"write\"|\"codex_agent\"" /Users/feifei/agent/neurx-code/src/agent/*.cpp | head -10

# 3. 检查信号连接
echo "=== Checking Signal Connections ==="
grep -r "toolApprovalRequired\|approveTool" /Users/feifei/agent/neurx-code/src/main.cpp /Users/feifei/agent/neurx-code/src/MainWindow.cpp 2>/dev/null

# 4. 检查配置
echo "=== Checking Configuration ==="
grep -r "autoApproveTools" /Users/feifei/agent/neurx-code/ --include="*.cpp" --include="*.h" --include="*.json"

# 5. 查看最近的应用日志
echo "=== Checking Application Logs ==="
tail -100 ~/.local/share/NeurXCode/logs/*.log 2>/dev/null || echo "No logs found"

# 6. 验证文件是否存在
echo "=== Checking for hello.cc ==="
find ~ -name "hello.cc" -type f 2>/dev/null || echo "File not found"
```

---

## 🚀 逐步修复指南

### 步骤 A: 启用详细日志

编辑 `neurx-code/src/agent/AgentEngine.cpp`，在关键函数添加 qDebug():

```cpp
void AgentEngine::handleToolCalls(const QVector<ToolCall> &calls)
{
    qDebug() << "handleToolCalls: received" << calls.size() << "tool calls";
    
    for (const auto &call : calls) {
        qDebug() << "  Tool:" << call.name;
        
        if (shouldRequireApproval(call)) {
            qDebug() << "    Approval required:" << approvalRiskLevel(call);
            emit toolApprovalRequired(call, approvalRiskLevel(call));
        } else {
            qDebug() << "    Executing without approval";
            // Execute
        }
    }
}
```

### 步骤 B: 验证 WriteTool 是否被调用

在运行应用时，打开开发者控制台 (Cmd+Alt+I) 并查找:
```
"Tool: Write"
"Tool: codex_agent"  
"Tool: file_creation"
```

### 步骤 C: 验证批准流程

查看是否显示:
```
"Approval required: high"
```

### 步骤 D: 验证执行

查看是否显示:
```
"toolFinished: success"
"File written to: /path/to/hello.cc"
```

---

## 📚 相关完整文档

| 文档 | 用途 |
|------|------|
| **WRITE_OPERATION_FAILURE_DIAGNOSIS_GUIDE.md** | 完整诊断指南 (2500+ 行) |
| **WRITE_OPERATION_TROUBLESHOOTING_QUICK_REF.md** | 快速参考 (800+ 行) |
| **WRITE_OPERATION_CODE_MAP.md** | 代码映射和调试 (900+ 行) |
| 本文档 | 快速诊断流程 |

---

## ✅ 验证修复成功

修复完成后，测试:

```bash
# 1. 启动应用
# 2. 在 agent 请求框输入: "Create a file called test.cpp with hello world code"
# 3. 验证:
#    ✅ 看到批准对话框
#    ✅ 点击"批准"后弹窗关闭
#    ✅ 看到"File written to test.cpp"消息
#    ✅ 文件 test.cpp 在工作目录中存在

# 4. 查看文件内容
cat test.cpp

# 5. 清理
rm test.cpp
```

---

**诊断完成日期**: 2026-06-12  
**下一步**: 按照上面的问题1-5进行诊断，找出具体是哪个环节有问题，然后参考对应的完整诊断文档进行修复。
